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
#ifndef UTIL_ADD_FST_TO_TRIE_H
#define UTIL_ADD_FST_TO_TRIE_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fstrain/util/print-fst.h"

namespace fstrain { namespace create {

namespace nsAddFstToTrieUtil {

template<class Arc>
void AddFstToTrie0(const fst::Fst<Arc>& fst, typename Arc::StateId fst_state, 
		   fst::MutableFst<Arc>* trie, typename Arc::StateId trie_state,
                   typename Arc::Weight incoming_weight) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  trie->SetFinal(trie_state, Weight::One());
  for(fst::ArcIterator<fst::Fst<Arc> > fst_iter(fst, fst_state); 
      !fst_iter.Done(); fst_iter.Next()) {
    const Arc& fst_arc = fst_iter.Value();
    Label fst_label = fst_arc.ilabel;
    fst::MutableArcIterator<fst::MutableFst<Arc> > trie_iter(trie, trie_state);
    for(; !trie_iter.Done(); trie_iter.Next()) {
      if(trie_iter.Value().ilabel == fst_label){
        break;
      }
    }
    bool found_fst_label = !trie_iter.Done();
    StateId trie_nextstate = fst::kNoStateId;
    Weight count = Times(fst_arc.weight, fst.Final(fst_arc.nextstate));
    if(found_fst_label) {
      Arc trie_arc = trie_iter.Value();
      trie_arc.weight = Plus(trie_arc.weight, count);
      trie_iter.SetValue(trie_arc);
      trie_nextstate = trie_arc.nextstate;
    }
    else {
      trie_nextstate = trie->AddState();
      trie->AddArc(trie_state,
                   Arc(fst_label, fst_label, count, trie_nextstate));
    }
    AddFstToTrie0(fst, fst_arc.nextstate, 
                  trie, trie_nextstate,
                  Times(incoming_weight, fst_arc.weight));
  }
}

} // end namespace


/**
 * @brief Adds a count FST to a trie, which encodes ngram counts. 
 *
 * Example counts FST:
 * 0 1 a 3
 * 1 2 b 1/3
 * 1 2 c 2/3
 * 1 (stop) 1
 * 2 (stop) 1
 */
template<class Arc>
void AddFstToTrie(const fst::Fst<Arc>& fst, fst::MutableFst<Arc>* trie) {
  typedef typename Arc::Weight Weight;
  if(fst.Start() != fst::kNoStateId){
    if(trie->Start() == fst::kNoStateId) {
      typename Arc::StateId s = trie->AddState();
      trie->SetStart(s);      
    }
    nsAddFstToTrieUtil::AddFstToTrie0(fst, fst.Start(), trie, trie->Start(), 
                                      Weight::One());
  }
}

} } // end namespaces fstrain::create

#endif
