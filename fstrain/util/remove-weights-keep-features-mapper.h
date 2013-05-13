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
#ifndef FSTRAIN_UTIL_REMOVE_WEIGHTS_KEEP_FEATURES_MAPPER_H
#define FSTRAIN_UTIL_REMOVE_WEIGHTS_KEEP_FEATURES_MAPPER_H

namespace fstrain { namespace util {

/**
 * @brief Mapper to set all non-zero weights to One() (but keep all features
 * in).
 */
template <class Arc>
class RemoveWeightsKeepFeaturesMapper {

  typedef typename Arc::Weight Weight;
  typedef typename Weight::MDExpectations MDExpectations;

 public:

  Arc operator()(const Arc &arc) const {    
    using namespace fst;
    Arc new_arc = arc;
    if(new_arc.weight != Weight::Zero()){
      new_arc.weight = Divide(arc.weight, 
			      Weight(arc.weight.Value()));
    }
    return new_arc;
  }

  fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }

  fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const { return props; }

};

} } // end namespaces

#endif
