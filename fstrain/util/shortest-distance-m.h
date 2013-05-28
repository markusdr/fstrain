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

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fst/reverse.h"
#include "fst/float-weight.h"
#include "fstrain/util/debug.h"
#include <vector>

namespace fstrain { namespace util {

/**
 * @brief Adds the first to the second weight vector.
 */
template<class Arc>
void Add(const std::vector<typename Arc::Weight>& vec1,
         std::vector<typename Arc::Weight>* vec2) {
  assert(vec1.size() == vec2->size());
  for(std::size_t i = 0; i < vec1.size(); ++i) {
    (*vec2)[i] = Plus(vec1[i], (*vec2)[i]);
  }
}

/**
 * @brief Moves one step further from the current distances. This
 * is a matrix multiplication Ax.
 */
template<class Arc>
void DoOneStep(const fst::Fst<Arc>& fst,
               std::vector<typename Arc::Weight>* curr_distances,
               double delta) {
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  using namespace fst;
  std::vector<Weight> next_distances(curr_distances->size(), Weight::Zero());
  for(StateIterator< Fst<Arc> > siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    assert(curr_distances->size() > s || (!(std::cerr << curr_distances->size() << " vs " << s << std::endl)));
    if((*curr_distances)[s] == Weight::Zero()){
      continue;
    }
    for(ArcIterator< Fst<Arc> > aiter(fst, s); !aiter.Done(); aiter.Next()){
      const Arc& arc = aiter.Value();
      assert(next_distances.size() > arc.nextstate);
      Weight sum = Plus(next_distances[arc.nextstate],
                        Times((*curr_distances)[s], arc.weight));
      next_distances[arc.nextstate] = sum;
    }
  }
  std::copy(next_distances.begin(), next_distances.end(),
            curr_distances->begin());
}

template<class Arc>
std::size_t DetermineNumStates(const fst::Fst<Arc>& fst) {
  std::size_t result = 0;
  for(fst::StateIterator< fst::Fst<Arc> > siter(fst); !siter.Done();
      siter.Next()) {
    ++result;
  }
  return result;
}

/**
 * @brief Return the sum of the weight of all successful paths in an
 * FST, i.e., the shortest-distance from the initial state to the
 * final states.
 *
 * @return true if converged
 */
template <class Arc>
bool ShortestDistanceM(const fst::Fst<Arc>& fst,
                       std::vector<typename Arc::Weight> *distance,
                       bool reverse = false,
                       double delta = fst::kDelta,
                       std::size_t miniter = 100,
                       std::size_t maxiter = 1000) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  std::size_t num_states = DetermineNumStates(fst);
  // std::cerr << "Num states: " << num_states << std::endl;
  std::vector<Weight> tmp;

  const fst::Fst<Arc>* fst_ptr = &fst;
  fst::MutableFst<Arc>* reversed = NULL;
  if(reverse) {
    reversed = new fst::VectorFst<Arc>();
    fst::Reverse(fst, reversed);
    fst_ptr = reversed;
    distance->resize(num_states + 1, Weight::Zero());
    tmp.resize(num_states + 1, Weight::Zero());
  }
  else {
    distance->resize(num_states, Weight::Zero());
    tmp.resize(num_states, Weight::Zero());
  }

  StateId s = fst_ptr->Start();
  (*distance)[s] = Weight::One();
  tmp[s] = Weight::One();

  // TEST
  fst::MutableFst<Arc>* tmpfst = new fst::VectorFst<Arc>();
  *tmpfst = *fst_ptr;
  tmpfst->Write("foo.fst");
  delete tmpfst;

  bool converged = false;
  Weight prev_sum(Weight::Zero());
  int iter = 0;
  while(iter < miniter || iter < maxiter) {
    DoOneStep(*fst_ptr, &tmp, delta);
    Add<Arc>(tmp, distance);
    Weight sum(Weight::Zero());
    for(typename std::vector<Weight>::const_iterator it = distance->begin();
        it != distance->end(); ++it) {
      sum = Plus(sum, *it);
    }
    if(fst::ApproxEqual(prev_sum, sum, delta)) {
      FSTR_UTIL_DBG_MSG(10, "ShortestDistanceM converged after "
                        << iter << " iterations." << std::endl);
      converged = true;
      //break;
    }
    prev_sum = sum;
    ++iter;
  }
  delete reversed;
  if(!converged) {
    std::cerr << "Not converged after " << maxiter << " iterations." << std::endl;
  }
  if(reverse) {
    for(std::size_t i = 1; i < distance->size(); ++i) {
      (*distance)[i-1] = (*distance)[i].Reverse();
    }
    distance->resize(distance->size() - 1);
  }
  return converged;
}

} } // end namespaces
