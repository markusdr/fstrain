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
#ifndef FSTRAIN_TRAIN_LENMATCH_H
#define FSTRAIN_TRAIN_LENMATCH_H

#include <cassert>
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/expectations.h"
#include "obj-func-fst-util.h"
#include "obj-func-fst.h"
#include "fstrain/train/debug.h"
#include "fstrain/train/length-feat-mapper.h"
//#include "fstrain/train/shortest-distance-timeout.h"
//#include "fstrain/util/double-precision-weight.h"
//#include "fstrain/util/memory-info.h"
#include "fstrain/util/print-fst.h"
//#include "fstrain/util/string-to-fst.h"
//#include "fstrain/util/timer.h"

namespace fstrain { namespace train {

namespace nsAddLengthRegularizationUtil {

  void SetAllGradientsTo(double* g, int num_params, double val);

} // end namespace

struct AddLengthRegularizationOpts {
  double* gradients;
  int num_params;
  const fstrain::core::MDExpectations& empirical_lengths;
  long* shortestdistance_timelimit;
  double fst_delta;
  fstrain::core::MDExpectations* expected_lengths;
  double length_variance;

  // constructor
  explicit AddLengthRegularizationOpts(const fstrain::core::MDExpectations& empirical_lengths_)
    : empirical_lengths(empirical_lengths_),
      fst_delta(1e-8),
      length_variance(1.0)
  {}
};

template<class TLengthFeatMapper>
double AddLengthRegularization(const fst::Fst<fst::MDExpectationArc>& model_fst,
                               const AddLengthRegularizationOpts& opts) {
  using namespace nsAddLengthRegularizationUtil;
  using fstrain::core::NeglogNum;
  NeglogNum retval(core::kPosInfinity); // -log(0.0)
  typedef fst::MapFst<fst::MDExpectationArc, fst::MDExpectationArc, TLengthFeatMapper> MapFstLengthFeats;
  TLengthFeatMapper len_mapper;
  MapFstLengthFeats fst_mapped(model_fst, len_mapper);
  using nsObjectiveFunctionFstUtil::TimedShortestDistance;
  typedef fst::MDExpectationArc::Weight Weight;
  typedef fst::MDExpectationArc::StateId StateId;

  std::vector<Weight> alphas;
  bool success1 = TimedShortestDistance(fst_mapped, &alphas, false,
                                        opts.fst_delta,
                                        opts.shortestdistance_timelimit);
  if(!success1) {
    SetAllGradientsTo(opts.gradients, opts.num_params, 0.0);
    opts.expected_lengths->insert(0, NeglogNum(core::kNegInfinity));
    return core::kPosInfinity; // infinite length -> infinite cost
  }
  if(alphas.size() == 0){
    throw std::runtime_error("no paths: bad fst");
  }

  // long unlimited = -1; // if we reached this the FST converges [not
  // true]
  std::vector<Weight> betas;
  bool success2 = TimedShortestDistance(fst_mapped, &betas, true,
                                        opts.fst_delta,
                                        opts.shortestdistance_timelimit);
  if(!success2){
    SetAllGradientsTo(opts.gradients, opts.num_params, 0.0);
    opts.expected_lengths->insert(0, NeglogNum(core::kNegInfinity));
    return core::kPosInfinity;
  }
  if(betas.size() == 0){
    throw std::runtime_error("no paths: bad fst");
  }

  using fstrain::core::MDExpectations;
  using fst::LogWeight;

  StateId start_state = model_fst.Start();

  FSTR_TRAIN_DBG_EXEC(10, std::cerr << "Mapped FST:" << std::endl;
		      util::printFst(&fst_mapped, NULL, NULL, false, std::cerr));
  FSTR_TRAIN_DBG_MSG(10, "betas[start]=" << betas[start_state] << std::endl);

  NeglogNum pathsum = betas[start_state].Value();

  MDExpectations::divideBy(betas[start_state].GetMDExpectations(), pathsum,
                         *opts.expected_lengths);

  for(MDExpectations::const_iterator it = opts.expected_lengths->begin();
      it != opts.expected_lengths->end(); ++it){
    assert(it->second.sign_x); // length is positive
    retval = NeglogPlus(retval, it->second);
  }

  // Simultaneously walk through original FST and the FST mapped to
  // contain length features
  fst::StateIterator< fst::Fst<fst::MDExpectationArc> > siter1(model_fst);
  fst::StateIterator< fst::Fst<fst::MDExpectationArc> > siter2(fst_mapped);
  typedef fst::LogArc::StateId StateId;
  for (; !siter1.Done(); siter1.Next(), siter2.Next()){
    StateId in = siter1.Value();
    FSTR_TRAIN_DBG_MSG(10, "FROM STATE " << in << std::endl);
    bool in_state_ok = alphas.size() > in && alphas[in].Value() != core::kPosInfinity
	&& alphas[in].Value() == alphas[in].Value(); // fails for NaN
    if(!in_state_ok) {
      // std::cerr << "Warning (lenmatch.h): State " << in << " not reachable."
      //           << std::endl;
      continue;
    }
    fst::ArcIterator<fst::Fst<fst::MDExpectationArc> > aiter1(model_fst, in);
    fst::ArcIterator<fst::Fst<fst::MDExpectationArc> > aiter2(fst_mapped, in);
    for (; !aiter1.Done(); aiter1.Next(), aiter2.Next()){
      StateId out = aiter1.Value().nextstate;
      StateId out2 = aiter2.Value().nextstate;
      assert(out == out2);
      bool out_state_ok = betas.size() > out && betas[out].Value() != core::kPosInfinity
	&& betas[out].Value() == betas[out].Value(); // fails for NaN
      if(out_state_ok) {
	Weight expected_lengths_w = Times(Times(alphas[in], aiter2.Value().weight), betas[out]);
        const MDExpectations& expected_lengths0 = expected_lengths_w.GetMDExpectations();
        MDExpectations expected_lengths;
	MDExpectations::divideBy(expected_lengths0, expected_lengths_w.Value(), expected_lengths); // g
	NeglogNum p(NeglogDivide(expected_lengths_w.Value(), pathsum));
	FSTR_TRAIN_DBG_MSG(10, "p=" << GetOrigNum(p) << std::endl);
        double one_over_variance = 1.0 / opts.length_variance;
	FSTR_TRAIN_DBG_MSG(10, "one_over_variance = " << one_over_variance << std::endl);
        const MDExpectations& feat_expectations0 = aiter1.Value().weight.GetMDExpectations();
	MDExpectations feat_expectations;
	MDExpectations::divideBy(feat_expectations0, NeglogNum(aiter1.Value().weight.Value()),
			       feat_expectations);
        for(MDExpectations::const_iterator it = feat_expectations.begin();
            it != feat_expectations.end(); ++it){
          int feat_index = it->first;
          double feat_fire_cnt = GetOrigNum(it->second);
	  NeglogNum fp(NeglogTimes(p, -log(feat_fire_cnt)));
	  FSTR_TRAIN_DBG_MSG(10, "feat_fire_cnt["<<feat_index<<"]=" << feat_fire_cnt << std::endl);
          for(MDExpectations::const_iterator it2 = opts.expected_lengths->begin();
              it2 != opts.expected_lengths->end(); ++it2){
	    double length_diff_overall = GetOrigNum((*opts.expected_lengths)[it2->first])
	      - GetOrigNum(opts.empirical_lengths[it2->first]);
	    FSTR_TRAIN_DBG_MSG(10, "E[len thru this arc] = " << GetOrigNum(expected_lengths[it2->first]) << std::endl);
	    double length_diff_this_arc =
	      GetOrigNum(expected_lengths[it2->first]) - GetOrigNum((*opts.expected_lengths)[it2->first]);
	    FSTR_TRAIN_DBG_MSG(10, "length_diff_overall = " << length_diff_overall << std::endl;);
	    FSTR_TRAIN_DBG_MSG(10, "length_diff_this_arc = " << length_diff_this_arc << std::endl;);
            // double add_term = feat_fire_cnt * GetOrigNum(p) * length_diff_this_arc
	    double add_term = GetOrigNum(fp) * length_diff_this_arc
	      * one_over_variance * length_diff_overall;
	    opts.gradients[feat_index] -= add_term;
            FSTR_TRAIN_DBG_MSG(10, "g["<<feat_index<<"] += " << add_term << std::endl);
          }
        }
      }
    }
  }
  assert(retval.sign_x);
  return GetOrigNum(retval);
}

} } // end namespace fstrain::train

#endif
