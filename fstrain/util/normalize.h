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
#ifndef FSTRAIN_UTIL_NORMALIZE_H
#define FSTRAIN_UTIL_NORMALIZE_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/concat.h"
#include "fst/shortest-distance.h"
#include <vector>
#include "fstrain/create/debug.h"

namespace fstrain { namespace util {

template<class Arc>
void Normalize(fst::MutableFst<Arc>* the_fst, typename Arc::Weight pathsum) {
  using namespace fst;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  const bool already_normalized = (pathsum == Weight::One());
  if (!already_normalized) {
    VectorFst<Arc> tmp;
    StateId s1 = tmp.AddState();
    tmp.SetStart(s1);
    StateId s2 = tmp.AddState();
    const int64 kEpsLabel = 0;
    tmp.AddArc(s1,
               Arc(kEpsLabel, kEpsLabel, Divide(Weight::One(), pathsum), s2));
    tmp.SetFinal(s2, Weight::One());
    Concat(tmp, the_fst);
  }
}

template<class Arc>
typename Arc::Weight ComputePathsum(const fst::Fst<Arc>& the_fst) {
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  std::vector<Weight> betas;
  const bool reverse = true;
  FSTR_UTIL_DBG_EXEC(10, std::cerr << "ComputePathsum" << std::endl; printTransducer(&the_fst, NULL, NULL, std::cerr));
  fst::ShortestDistance(the_fst, &betas, reverse);
  if (betas.size() == 0) {
    FSTR_CREATE_EXCEPTION("no betas found");
  }
  StateId start_state = the_fst.Start();
  return betas[start_state];
}

/**
 * @brief Makes all paths sum to one.
 */
template<class Arc>
void Normalize(fst::MutableFst<Arc>* the_fst) {
  Normalize(the_fst, ComputePathsum(*the_fst));
}

} } // end namespaces

#endif
