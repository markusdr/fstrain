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
#ifndef FSTRAIN_CREATE_ADD_UNOBSERVED_UNIGRAMS_H
#define FSTRAIN_CREATE_ADD_UNOBSERVED_UNIGRAMS_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"

namespace fstrain { namespace create {

/**
 * @brief Adds to the ngram count trie all unigrams that have not been
 * observed otherwise.
 */
template<class Arc>
void AddUnobservedUnigrams(const fst::SymbolTable& syms,
                           typename Arc::Weight weight,
                           fst::MutableFst<Arc>* trie) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  StateId start = trie->Start();
  std::set<int64> have_syms;
  for (fst::ArcIterator<fst::Fst<Arc> > ait(*trie, start);
      !ait.Done(); ait.Next()) {
    have_syms.insert(ait.Value().ilabel);
  }
  fst::SymbolTableIterator sit(syms);
  sit.Next();  // ignore eps
  for (; !sit.Done(); sit.Next()) {
    int64 label = sit.Value();
    if (have_syms.find(label) == have_syms.end()) {
      StateId new_state = trie->AddState();
      trie->SetFinal(new_state, Weight::One());
      trie->AddArc(start,
                   Arc(label, label, weight, new_state));
    }
  }
}

} } // end namespace

#endif
