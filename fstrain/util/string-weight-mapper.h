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
#ifndef FSTR_UTIL_STRING_WEIGHT_MAPPER_H
#define FSTR_UTIL_STRING_WEIGHT_MAPPER_H

#include "fst/arc.h"
#include "fst/string-weight.h"
#include "fst/project.h"

namespace fst {

// Mapper from A to StringArc<S>.
template <class A, StringType S = STRING_LEFT>
struct ToStringMapper {
  typedef A FromArc;
  typedef StringArc<S> ToArc;

  typedef StringWeight<typename A::Label, S> SW;
  typedef typename A::Weight AW;

  ToArc operator()(const A &arc) const {
    // 'Super-final' arc.
    if (arc.nextstate == kNoStateId && arc.weight != AW::Zero())
      return ToArc(0, 0, SW::One(), kNoStateId);
    // 'Super-non-final' arc.
    else if (arc.nextstate == kNoStateId)
      return ToArc(0, 0, SW::Zero(), kNoStateId);
    // Epsilon label.
    else if (arc.olabel == 0)
      return ToArc(arc.ilabel, arc.ilabel,
                   SW::One(), arc.nextstate);
    // Regular label.
    else
      return ToArc(arc.ilabel, arc.ilabel,
                   SW(arc.olabel), arc.nextstate);
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_CLEAR_SYMBOLS;}

  uint64 Properties(uint64 props) const {
    return ProjectProperties(props, true) & kWeightInvariantProperties;
  }
};

} // end namespace

#endif
