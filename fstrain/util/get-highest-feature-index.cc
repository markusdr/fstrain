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
#include <cmath>
#include "fst/fst.h"
#include "fstrain/util/get-highest-feature-index.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace util {

  typedef int StateId;
  using namespace fst;

  void processWeight(const MDExpectationArc::Weight& w, int& result) {
    typedef MDExpectationArc::Weight::MDExpectations MDExpectations;
    const MDExpectations& e = w.GetMDExpectations();
    for (MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it) {
      int featureIndex = it->first;
      result = std::max(result, featureIndex);
    }
  }

  int getHighestFeatureIndex(const Fst<MDExpectationArc>& theFst) {
    int result = 0;
    for (StateIterator< Fst<MDExpectationArc> > siter(theFst); !siter.Done(); siter.Next()) {
      StateId in = siter.Value();
      for (ArcIterator<Fst<MDExpectationArc> > aiter(theFst, in); !aiter.Done(); aiter.Next()) {
	processWeight(aiter.Value().weight, result);
      }
      processWeight(theFst.Final(in), result);
    }
    return result;
  }

} } // end namespace fstrain::util
