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
#ifndef FSTRAIN_TRAIN_LENGTH_FEAT_MAPPER_H
#define FSTRAIN_TRAIN_LENGTH_FEAT_MAPPER_H

#include "fst/map.h"
#include "fstrain/train/debug.h"
#include "fstrain/core/mutable-weight.h"

namespace fstrain { namespace train {

/**
 * @brief Mapper to insert length features
 */
template <class Arc>
struct LengthFeatMapper_XY {
  typedef typename Arc::Weight Weight;

  enum FeatureName {LENGTH_X, LENGTH_Y};

  explicit LengthFeatMapper_XY()
  {}

  Arc operator()(const Arc &arc) const {
    Arc new_arc;
    new_arc.ilabel = arc.ilabel;
    new_arc.olabel = arc.olabel;
    new_arc.nextstate = arc.nextstate;

    Weight& w = new_arc.weight;
    // w.SetValue(arc.weight.Value());
    static_cast<core::MutableWeight<Weight>&>(w).SetValue(arc.weight.Value());

    fstrain::core::MDExpectations& e = w.GetMDExpectations();
    if(arc.ilabel != 0){
      e.insert(LENGTH_X, w.Value());
    }
    if(arc.olabel != 0){
      e.insert(LENGTH_Y, w.Value());
    }
    return new_arc;
  }

  int GetNumFeats() {
    return 2;
  }

  fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }

  fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const {
    return props;
  }

};

/**
 * @brief Mapper to insert length features
 */
template <class Arc>
struct LengthFeatMapper_Y {
  typedef typename Arc::Weight Weight;

  explicit LengthFeatMapper_Y()
  {}

  Arc operator()(const Arc &arc) const {
    Arc new_arc;
    new_arc.ilabel = arc.ilabel;
    new_arc.olabel = arc.olabel;
    new_arc.nextstate = arc.nextstate;

    Weight& w = new_arc.weight;
    static_cast<core::MutableWeight<Weight>&>(w).SetValue(arc.weight.Value());

    fstrain::core::MDExpectations& e = w.GetMDExpectations();
    if(arc.olabel != 0){
      int index = 0;
      e.insert(index, w.Value());
    }
    return new_arc;
  }

  int GetNumFeats() {
    return 1;
  }

  fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }

  fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const {
    return props;
  }

};

} } // end namespace fstrain::train

#endif
