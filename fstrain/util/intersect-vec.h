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
#ifndef FSTR_UTIL_INTERSECT_VEC_H
#define FSTR_UTIL_INTERSECT_VEC_H

#include <vector>
#include "fst/intersect.h"
#include "fst/minimize.h"
#include "fst/mutable-fst.h"
#include "fst/rmepsilon.h"
#include "fst/vector-fst.h"
#include "fstrain/util/determinize-in-place.h"

namespace fstrain { namespace util {

template<class Arc>
void MyIntersect(const fst::Fst<Arc>& ifst1,
                 const fst::Fst<Arc>& ifst2,
                 fst::MutableFst<Arc>* ofst,
                 bool do_determinize) {
  fst::Intersect(ifst1, ifst2, ofst);
  if (do_determinize) {
    fst::RmEpsilon(ofst);
    fstrain::util::Determinize(ofst);
    fst::Minimize(ofst);
  }
}

template<class Arc>
void Intersect_vec(const std::vector<fst::Fst<Arc>*>& ifsts,
                   fst::MutableFst<Arc>* ofst,
                   bool determinizeIntermediate = true) {
  if (ifsts.size() == 0) {
    return;
  }
  if (ifsts.size() == 1) {
    *ofst = *(ifsts[0]);
    return;
  }
  typedef fst::ArcSortFst<Arc, fst::OLabelCompare<Arc> > MyArcSortFst;
  MyIntersect(MyArcSortFst(*ifsts[0], fst::OLabelCompare<Arc>()),
              *ifsts[1],
              ofst,
              determinizeIntermediate);
  for (int i = 2; i < ifsts.size(); ++i) {
    MyArcSortFst ofst_sorted(*ofst, fst::OLabelCompare<Arc>());
    MyIntersect(ofst_sorted, *ifsts[i], ofst, determinizeIntermediate);
  }
}

} } // end namespaces


#endif
