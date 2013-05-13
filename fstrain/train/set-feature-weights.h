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
/**
 * $Id: set-feature-weights.h 5948 2009-06-23 18:07:59Z markus $
 *
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_TRAIN_SET_FEATURE_WEIGHTS_H_
#define FSTRAIN_TRAIN_SET_FEATURE_WEIGHTS_H_

#include <map>
#include "fst/mutable-fst.h"
#include "fst/map.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace train {

template<class FloatT>
class SetFeatureWeightsMapper {

 public:

  explicit
  SetFeatureWeightsMapper(const FloatT* weights);

  fst::MDExpectationArc operator()(const fst::MDExpectationArc& arc) const;

  fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }

  fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const { return props; }

 private:

  void SetFeatures(fst::MDExpectationWeight* weight) const;

  const FloatT* weights_;
};

/**
 * Sets the arc weights and feature expectations in an fst from a feature weights vector.
 */
template<class FloatT>
void SetFeatureWeights(const FloatT* weights, fst::MutableFst<fst::MDExpectationArc>* fst);


//
// Definitions
//


template<class FloatT>
SetFeatureWeightsMapper<FloatT>::SetFeatureWeightsMapper(const FloatT* weights)
    : weights_(weights) {}

template<class FloatT>
fst::MDExpectationArc SetFeatureWeightsMapper<FloatT>::operator()(const fst::MDExpectationArc& arc) const {
  fst::MDExpectationWeight new_weight(arc.weight);
  SetFeatures(&new_weight);
  return fst::MDExpectationArc(arc.ilabel, arc.olabel, new_weight, arc.nextstate);
}

template<class FloatT>
void SetFeatureWeightsMapper<FloatT>::SetFeatures(fst::MDExpectationWeight* weight) const {
  using namespace fstrain::core;
  MDExpectations& e = weight->GetMDExpectations();
  if(e.size()){
    NeglogNum old_value = weight->Value();
    NeglogNum arc_weight = NeglogNum(0.0);
    std::map<int, NeglogNum> feat_cnt_fire_neglog_map;
    for(MDExpectations::iterator it = e.begin(); it != e.end(); ++it){
      int feat_index = it->first;
      NeglogNum feat_cnt_fire_neglog(NeglogDivide(it->second, old_value));
      feat_cnt_fire_neglog_map[feat_index] = feat_cnt_fire_neglog;
      double feat_cnt_fire = GetOrigNum(feat_cnt_fire_neglog);
      arc_weight = NeglogTimes(arc_weight,
                               feat_cnt_fire * weights_[feat_index]);
    }
    for(MDExpectations::iterator it = e.begin(); it != e.end(); ++it){
      it->second = NeglogTimes(arc_weight, feat_cnt_fire_neglog_map[it->first]);
    }
    assert(arc_weight.sign_x); // represents a positive num
    // weight->SetValue(arc_weight.lx);
    double const& val = weight->Value();
    const_cast<double&>(val) = arc_weight.lx; // HACK
  }
}

template<class FloatT>
void SetFeatureWeights(const FloatT* weights, fst::MutableFst<fst::MDExpectationArc>* fst) {
  SetFeatureWeightsMapper<FloatT> mapper(weights);
  Map(fst, mapper);
}

} } // end namespace fstrain::train

#endif // FSTRAIN_TRAIN_SET_FEATURE_WEIGHTS_H_
