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
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/expectations.h"
#include "fstrain/train/obj-func-fst-conditional-lenmatch.h"
#include "fstrain/train/obj-func-fst-util.h"
#include "fstrain/train/obj-func-fst.h"
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

ObjectiveFunctionFstConditionalLenmatch::ObjectiveFunctionFstConditionalLenmatch(
    fst::MutableFst<fst::MDExpectationArc>* fst,
    const util::Data* data,
    const fst::SymbolTable* isymbols,
    const fst::SymbolTable* osymbols,
    double variance,
    double length_variance)
    : ObjectiveFunctionFst(fst),
      data_(data), isymbols_(isymbols), osymbols_(osymbols),
      variance_(variance), length_variance_(length_variance), match_xy_(true)
{
  std::cerr << "# Constructing ObjectiveFunctionFstConditionalLenmatch" << std::endl;
  std::cerr << "# Data size: " << data_->size() << std::endl;
  std::cerr << "# Num params: " << GetNumParameters() << std::endl;
}

ObjectiveFunctionFstConditionalLenmatch::~ObjectiveFunctionFstConditionalLenmatch() {
  delete data_;
  delete isymbols_;
  delete osymbols_;
}

void ObjectiveFunctionFstConditionalLenmatch::SetLengthVariance(double variance) {
  length_variance_ = variance;
}

double ObjectiveFunctionFstConditionalLenmatch::GetLengthVariance() {
  return length_variance_;
}

void ObjectiveFunctionFstConditionalLenmatch::SetMatchXY(bool val) {
  std::cerr << "Match X and Y length? " << (val ? "YES" : "NO") << std::endl;
  match_xy_ = val;
}

bool ObjectiveFunctionFstConditionalLenmatch::GetMatchXY() {
  return match_xy_;
}

void ObjectiveFunctionFstConditionalLenmatch::ComputeGradientsAndFunctionValue(const double* x) {

  static int call_counter = -1;
  ++call_counter;
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  using core::NeglogNum;
  using core::GetOrigNum;

  FSTR_TRAIN_DBG_EXEC(10,
                     for(int i = 0; i < GetNumParameters(); ++i){
                       std::cerr << "x["<<i<<"] = " << x[i]<< std::endl;
                     });

  util::Timer timer;
  size_t num_params = GetNumParameters();
  double* gradients = GetGradients();

  SetFunctionValue(0.0);
  for(size_t i = 0; i < num_params; ++i){
    gradients[i] = 0.0;
  }

  // regularize
  double norm = 0.0;
  for(int i = 0; i < num_params; ++i){
    norm += (x[i] * x[i]);
    gradients[i] += x[i] / variance_;
  }
  SetFunctionValue(GetFunctionValue() + norm / (2.0 * variance_));

  for(size_t i = 0; i < data_->size(); ++i) {
    if(exclude_data_indices_.find(i) != exclude_data_indices_.end()){
      continue;
    }
    const std::pair<std::string, std::string>& inout = (*data_)[i];
    try {
      ProcessInputOutputPair(inout.first, inout.second, call_counter);
      if(GetFunctionValue() == core::kPosInfinity){
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
  if(add_length_regularization){
    const int n = data_->size() - exclude_data_indices_.size();
    double len0 = 0.0;
    double len1 = 0.0;
    double empirical_length_sum = 0.0;
    for(size_t i = 0; i < data_->size(); ++i) {
      if(exclude_data_indices_.find(i) != exclude_data_indices_.end()){
        continue;
      }
      const std::pair<std::string, std::string>& inout = (*data_)[i];
      double len_in = (inout.first.length() + 1) / 2 / (double)n; // "S a b E" = len 4, not 7
      double len_out = (inout.second.length() + 1) / 2 / (double)n;
      if(match_xy_){
	len0 += len_in;
	len1 += len_out;
	empirical_length_sum += len_in + len_out;
      }
      else {
	len0 += len_out;
	empirical_length_sum += len_out;
      }
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
    alrOpts.fst_delta = GetFstDelta();
    alrOpts.fst_delta = GetFstDelta();
    alrOpts.expected_lengths = &expected_lengths;
    alrOpts.length_variance = length_variance_;
    if(match_xy_){
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
    for(int i = 0; i < expected_lengths.size(); ++i){
      double penalty0 = GetOrigNum(expected_lengths[i]) - GetOrigNum(empirical_lengths[i]);
      double penalty = penalty0 * penalty0 / (2.0 * length_variance_);
      std::cerr << "g" << "["<<i<<"]: "
                << "Empirical: "<< GetOrigNum(empirical_lengths[i])
		<<", Expected:" << GetOrigNum(expected_lengths[i]) << ", Penalty: " << penalty
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

  if(GetFunctionValue() == core::kPosInfinity){
    SetFunctionValue(1.0);
    return;
  }

  SquashFunction();
}

void ObjectiveFunctionFstConditionalLenmatch::ProcessInputOutputPair(
    const std::string& in, const std::string& out, int iteration) {
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  VectorFst<MDExpectationArc> inputFst;
  VectorFst<MDExpectationArc> outputFst;
  util::ConvertStringToFst(in, *isymbols_, &inputFst);
  util::ConvertStringToFst(out, *osymbols_, &outputFst);
  assert(inputFst.InputSymbols() == NULL);
  assert(GetFst().InputSymbols() == NULL);
  ComposeFst<MDExpectationArc> unclamped(inputFst, GetFst());
  ComposeFstOptions<MDExpectationArc> copts;
  copts.gc_limit = 0;  // Cache only the last state for fastest copy.
  ComposeFst<MDExpectationArc> clamped(unclamped, outputFst, copts);
  double* gradients = GetGradients();
  // may throw:
  double clamped_result = GetFeatureMDExpectations<double, double*>(
      clamped, &gradients, GetNumParameters(),
      false, 1.0,
      GetFstDelta(), GetTimelimit());
  FSTR_TRAIN_DBG_MSG(10, "CLAMPED=" << clamped_result << std::endl);
  SetFunctionValue(GetFunctionValue() + clamped_result);
  long unlimited = -1;
  double unclamped_result =
      GetFeatureMDExpectations<double, double*>(
          unclamped, &gradients, GetNumParameters(),
          true, 1.0,
          GetFstDelta(), iteration == 0 ? &unlimited : GetTimelimit());
  FSTR_TRAIN_DBG_EXEC(100, util::printFst(&unclamped, NULL, NULL, false, std::cerr));
  FSTR_TRAIN_DBG_MSG(10, "UNCLAMPED=" << unclamped_result << std::endl);
  SetFunctionValue(GetFunctionValue() - unclamped_result);

}

} } // end namespace fstrain/train
