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
#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <string>
#include <set>

#include "fst/fst.h"
#include "fst/arc.h"

namespace fstrain { namespace util {

void ThrowExceptionIfFileNotFound(const std::string& filename);

template<class Arc>
bool HasOutArcs(const fst::Fst<Arc>& fst, typename Arc::StateId s) {
  fst::ArcIterator< fst::Fst<Arc> > ait(fst, s);
  return !ait.Done();
}

template<class Arc>
bool IsTrie(const fst::Fst<Arc>& fst) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  std::set<StateId> has_incoming_arc;
  for (StateIterator< Fst<Arc> > sit(fst); !sit.Done(); sit.Next()) {
    StateId s = sit.Value();
    for (ArcIterator< Fst<Arc> > ait(fst, s); !ait.Done(); ait.Next()) {
      StateId nextstate = ait.Value().nextstate;
      if (has_incoming_arc.find(nextstate) != has_incoming_arc.end()) {
        std::cerr << "State " << nextstate << " has multiple incoming arcs."
                  << std::endl;
        return false;
      }
      has_incoming_arc.insert(nextstate);
    }
  }
  return true;
} 

} } // end namespaces

#endif
