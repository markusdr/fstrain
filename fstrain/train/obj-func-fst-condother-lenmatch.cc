// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: markus.dreyer@gmail.com (Markus Dreyer)
//
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "fst/compose.h"
#include "fst/arcsort.h"
#include "fst/rmepsilon.h"
#include "fst/project.h"
#include "fst/union.h"
#include "fst/determinize.h"
#include "fst/shortest-path.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/expectations.h"
#include "obj-func-fst-condother-lenmatch.h"
#include "obj-func-fst-util.h"
#include "obj-func-fst.h"
#include "fstrain/train/debug.h"
#include "fstrain/train/length-feat-mapper.h"
#include "fstrain/train/lenmatch.h"
#include "fstrain/train/shortest-distance-timeout.h"
#include "fstrain/util/double-precision-weight.h"
#include "fstrain/util/memory-info.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/timer.h"

using namespace fst;

namespace fstrain { namespace train {

ObjectiveFunctionFstCondotherLenmatch::ObjectiveFunctionFstCondotherLenmatch(
    fst::MutableFst<fst::MDExpectationArc>* fst,
    const util::Data* data,
    const SymbolTable* isymbols,
    const SymbolTable* osymbols,
    double variance,
    double length_variance)
    : ObjectiveFunctionFstConditionalLenmatch(fst, data, isymbols, osymbols, variance, length_variance),
      data_(data),
      isymbols_(isymbols), osymbols_(osymbols),
      variance_(variance),
      other_list_(NULL), other_to_in_fst_(NULL), other_to_out_fst_(NULL),
      other_symbols_(NULL), kbest_(20)
{
  std::cerr << "# Constructing ObjectiveFunctionFstCondotherLenmatch" << std::endl;
  std::cerr << "# Data size: " << data_->size() << std::endl;
  std::cerr << "# Num params: " << GetNumParameters() << std::endl;
}

ObjectiveFunctionFstCondotherLenmatch::~ObjectiveFunctionFstCondotherLenmatch() {
  //  delete data_;
  //  data_ = NULL;
  //  delete isymbols_;
  //  isymbols_ = NULL;
  //  delete osymbols_;
  //  osymbols_ = NULL;
  delete other_symbols_;
  other_symbols_ = NULL;
  delete other_list_;
  other_list_ = NULL;
}

void ObjectiveFunctionFstCondotherLenmatch::SetOtherVariable(
    const std::vector<std::string>* other_list,
    const Fst<LogArc>* other_to_in_fst,
    const Fst<LogArc>* other_to_out_fst,
    const SymbolTable* other_symbols) {
  other_list_ = other_list;
  other_to_in_fst_ = other_to_in_fst;
  other_to_out_fst_ = other_to_out_fst;
  other_symbols_ = other_symbols;
}

void ObjectiveFunctionFstCondotherLenmatch::SetKbest(int kbest) {
  kbest_ = kbest;
}

void ObjectiveFunctionFstCondotherLenmatch::ComputeGradientsAndFunctionValue(const double* x) {

  static int call_counter = -1;
  ++call_counter;
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  using core::NeglogNum;
  using core::GetOrigNum;

  FSTR_TRAIN_DBG_EXEC(10,
                      for (int i = 0; i < GetNumParameters(); ++i) {
                        std::cerr << "x["<<i<<"] = " << x[i]<< std::endl;
                      });

  util::Timer timer;
  size_t num_params = GetNumParameters();
  double* gradients = GetGradients();

  SetFunctionValue(0.0);
  for (size_t i = 0; i < num_params; ++i) {
    gradients[i] = 0.0;
  }

  // regularize
  double norm = 0.0;
  for (int i = 0; i < num_params; ++i) {
    norm += (x[i] * x[i]);
    gradients[i] += x[i] / variance_;
  }
  SetFunctionValue(GetFunctionValue() + norm / (2.0 * variance_));

  for (size_t i = 0; i < data_->size(); ++i) {
    if (exclude_data_indices_.find(i) != exclude_data_indices_.end()) {
      continue;
    }
    const std::pair<std::string, std::string>& inout = (*data_)[i];
    try {
      const std::string other = (*other_list_)[i];
      ProcessInputOutputPair(inout.first, inout.second, other);
      if (GetFunctionValue() == core::kPosInfinity) {
	break;
      }
    }
    catch(std::runtime_error) {
      std::cerr << "Warning: Ignoring example "
                << inout.first << " / " << inout.second << std::endl;
      exclude_data_indices_.insert(i);
    }

  } // end loop over data

  // HACK
  const bool add_length_regularization = true;
  if (add_length_regularization) {
    const int n = data_->size() - exclude_data_indices_.size();
    double empirical_length_sum = 0.0;
    double len0 = 0.0;
    double len1 = 0.0;
    for (size_t i = 0; i < data_->size(); ++i) {
      if (exclude_data_indices_.find(i) != exclude_data_indices_.end()) {
        continue;
      }
      const std::pair<std::string, std::string>& inout = (*data_)[i];
      double len_in = (inout.first.length() + 1) / 2 / (double)n; // "S a b E" = len 4, not 7
      double len_out = (inout.second.length() + 1) / 2 / (double)n;
      if (GetMatchXY()) {
	len0 += len_in;
	len1 += len_out;
	empirical_length_sum += len_in + len_out;
      }
      else {
	len0 += len_out;
	empirical_length_sum += len_out;
      }
    }
    if (call_counter == 0) {
      SetTimelimit(-1); // disable
    }
    core::MDExpectations empirical_lengths;
    empirical_lengths.insert(0, NeglogNum(-log(len0)));
    empirical_lengths.insert(1, NeglogNum(-log(len1)));
    double expected_length = 0.0;
    core::MDExpectations expected_lengths;
    AddLengthRegularizationOpts alrOpts(empirical_lengths);
    alrOpts.gradients = GetGradients();
    alrOpts.num_params = GetNumParameters();
    alrOpts.shortestdistance_timelimit = GetTimelimit();
    alrOpts.expected_lengths = &expected_lengths;
    alrOpts.length_variance = GetLengthVariance();
    if (GetMatchXY()) {
      expected_length =
          AddLengthRegularization<LengthFeatMapper_XY<MDExpectationArc> >(GetFst(),
                                                                        alrOpts);
    }
    else {
      expected_length =
          AddLengthRegularization<LengthFeatMapper_Y<MDExpectationArc> >(GetFst(),
                                                                       alrOpts);
    }
    FSTR_TRAIN_DBG_MSG(10, "AddLengthRegularization: "<< expected_length << " - " << empirical_length_sum << std::endl);
    for (int i = 0; i < expected_lengths.size(); ++i) {
      double expected_len = GetOrigNum(expected_lengths[i]);
      double empirical_len = GetOrigNum(empirical_lengths[i]);
      double penalty0 = expected_len - empirical_len;
      double penalty = penalty0 * penalty0 / (2.0 * GetLengthVariance());
      std::cerr << "g" << "["<<i<<"]: "
                << "Empirical: "<< empirical_len <<", Expected:" << expected_len
                << ", Penalty: " << penalty
                << std::endl;
      SetFunctionValue(GetFunctionValue() + penalty);
    }
  }

  std::cerr << setprecision(8)
            << "Returning x=" << x[0] << "\tg=" << gradients[0]
            << "\tobj=" << GetFunctionValue();
  timer.stop();
  fprintf(stderr, "\t[%2.2f ms, %2.2f MB]\n",
          timer.get_elapsed_time_millis(), util::MemoryInfo::instance().getSizeInMB());

  if (GetFunctionValue() == core::kPosInfinity) {
    SetFunctionValue(1.0);
    return;
  }

  SquashFunction();
}

void ObjectiveFunctionFstCondotherLenmatch::ProcessInputOutputPair(
    const std::string& in, const std::string& out,
    const std::string& other) {
  static int call_counter = -1;
  ++call_counter;
  typedef WeightConvertMapper<LogArc, MDExpectationArc> Map_LE;
  typedef WeightConvertMapper<MDExpectationArc, StdArc> Map_ES;
  typedef WeightConvertMapper<StdArc, MDExpectationArc> Map_SE;
  typedef WeightConvertMapper<LogArc, StdArc> Map_LS;
  typedef WeightConvertMapper<StdArc, LogArc> Map_SL;
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  VectorFst<MDExpectationArc> inputFst;
  VectorFst<MDExpectationArc> outputFst;
  VectorFst<LogArc> other_fst;
  util::ConvertStringToFst(in, *isymbols_, &inputFst);
  util::ConvertStringToFst(out, *osymbols_, &outputFst);
  util::ConvertStringToFst(other, *other_symbols_, &other_fst);
  assert(inputFst.InputSymbols() == NULL);
  assert(GetFst().InputSymbols() == NULL);

  ComposeFstOptions<LogArc> copts_L;
  copts_L.gc_limit = 0;  // Cache only the last state for fastest copy.

  ProjectFst<LogArc> other_to_in(ComposeFst<LogArc>(other_fst, *other_to_in_fst_, copts_L),
                                 PROJECT_OUTPUT);
  ProjectFst<LogArc> other_to_out(ComposeFst<LogArc>(other_fst, *other_to_out_fst_, copts_L),
                                  PROJECT_OUTPUT);

  ArcSortFst<LogArc, OLabelCompare<LogArc> > other_to_in_sorted(other_to_in, OLabelCompare<LogArc>());
  ArcSortFst<LogArc, ILabelCompare<LogArc> > other_to_out_sorted(other_to_out, ILabelCompare<LogArc>());

  MapFst<LogArc, MDExpectationArc, Map_LE> other_to_in_final(other_to_in_sorted, Map_LE());
  MapFst<LogArc, MDExpectationArc, Map_LE> other_to_out_final(other_to_out_sorted, Map_LE());

  ComposeFstOptions<MDExpectationArc> copts;
  copts.gc_limit = 0;  // Cache only the last state for fastest copy.

  typedef ComposeFst<MDExpectationArc> EComposeFst;
  EComposeFst unclamped(EComposeFst(other_to_in_final, GetFst(), copts),
                        other_to_out_final, copts);
  EComposeFst clamped(EComposeFst(inputFst, unclamped, copts),
                      outputFst, copts);
  double* gradients = GetGradients();
  // may throw:
  double clamped_result = GetFeatureMDExpectations<double, double*>(
      clamped, &gradients, GetNumParameters(),
      false, 1.0,
      GetFstDelta(), GetTimelimit());
  FSTR_TRAIN_DBG_MSG(10, "CLAMPED=" << clamped_result << std::endl);
  SetFunctionValue(GetFunctionValue() + clamped_result);
  long unlimited = (long)1e8;
  util::Timer unclamped_timer;
  double unclamped_result =
      GetFeatureMDExpectations<double, double*>(
          unclamped, &gradients, GetNumParameters(),
          true, 1.0,
          GetFstDelta(), call_counter == 0 ? &unlimited : GetTimelimit());
  FSTR_TRAIN_DBG_EXEC(100, util::printFst(&unclamped, NULL, NULL, false, std::cerr));
  FSTR_TRAIN_DBG_MSG(10, "UNCLAMPED=" << unclamped_result << std::endl);
  unclamped_timer.stop();
  if (call_counter == 0) {
    double new_limit = 4.0 * unclamped_timer.get_elapsed_time_millis();
    SetTimelimit((long)new_limit);
  }
  SetFunctionValue(GetFunctionValue() - unclamped_result);

}

} } // end namespace fstrain/train
