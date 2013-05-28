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
 * @brief Implements k-best extraction from an FST.
 *
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_UTIL_KBEST_H
#define FSTRAIN_UTIL_KBEST_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <tr1/unordered_map>

#include "fst/shortest-path.h"
#include "fst/fst.h"

namespace fstrain { namespace util {

typedef std::pair<std::string, double> KbestEntry;
inline bool SmallerValuePred(const KbestEntry& e1, const KbestEntry& e2) {
  return e1.second < e2.second;
}


template<class A>
class KbestStrings {

  typedef std::pair<std::string, double> KbestEntry;
  typedef std::vector<KbestEntry> Container;
  typedef std::tr1::unordered_map<std::string, fst::LogWeight> KbestMap;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

 public:

  typedef Container::const_iterator const_iterator;

  KbestStrings(const fst::Fst<A>& fst,
               const fst::SymbolTable& symbols,
               unsigned kMax) {
    kbest_entries_.reserve(kMax);
    Init(fst, symbols, kMax);
  }

  void Normalize() {
    typedef fst::LogArc::Weight LogWeight;
    LogWeight sum = LogWeight::Zero();
    for(Container::const_iterator it = kbest_entries_.begin();
        it != kbest_entries_.end(); ++it) {
      sum = Plus(sum, LogWeight(it->second));
    }
    for(Container::iterator it = kbest_entries_.begin();
        it != kbest_entries_.end(); ++it) {
      it->second -= sum.Value();
    }
  }

  const_iterator begin() const { return kbest_entries_.begin(); }

  const_iterator end() const { return kbest_entries_.end(); }

  std::size_t size() const { return kbest_entries_.size(); }

 private:

  void ExpandState(const fst::Fst<A>& f,
                   const fst::SymbolTable& symbols,
                   StateId s,
                   std::string path,
                   Weight w,
                   KbestMap* the_map){
    for (fst::ArcIterator<fst::Fst<A> > aiter(f, s); !aiter.Done(); aiter.Next()){
      const A& a = aiter.Value();
      const std::string expanded_path = path +
          (a.ilabel > 0
           ? ((path.length() ? " " : "") + symbols.Find(a.ilabel))
           : "");
      ExpandState(f, symbols, a.nextstate, expanded_path, fst::Times(w, a.weight),
                  the_map);
    }
    if(f.Final(s) != Weight::Zero()){
      // kbest_entries_.push_back(std::make_pair(path, w.Value()));
      KbestMap::iterator found = the_map->find(path);
      const bool did_find = found != the_map->end();
      if(did_find) {
        found->second = fst::Plus(found->second, fst::LogWeight(w.Value()));
      }
      else {
        the_map->insert(std::make_pair(path, fst::LogWeight(w.Value())));
      }
    }
  }

  void Init(const fst::Fst<A>& f, const fst::SymbolTable& symbols, unsigned kMax){
    fst::VectorFst<A> kbestFst;
    const bool unique = false; // true not implemented by OpenFst
    fst::ShortestPath(f, &kbestFst, kMax, unique);
    std::string emptyPath = "";
    Weight zero = Weight::One();
    KbestMap the_map;
    ExpandState(kbestFst, symbols, kbestFst.Start(), emptyPath, zero, &the_map);
    for(KbestMap::const_iterator it = the_map.begin(); it != the_map.end(); ++it) {
      kbest_entries_.push_back(std::make_pair(it->first, it->second.Value()));
    }
    std::sort(kbest_entries_.begin(), kbest_entries_.end(), SmallerValuePred);
  }

  Container kbest_entries_;

};

} } // end namespace

#endif
