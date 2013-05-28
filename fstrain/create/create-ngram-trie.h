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
#ifndef FSTRAIN_CREATE_UTIL_CREATE_NGRAM_TRIE_H
#define FSTRAIN_CREATE_UTIL_CREATE_NGRAM_TRIE_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include <set>
#include <queue>

namespace fstrain { namespace create {

/**
 * @brief Options for CreateNgramTrie function.
 */
template<class Arc>
struct CreateNgramTrieOptions {
  typedef typename Arc::Label Label;
  const size_t kNgramOrder;
  const fst::SymbolTable& kSyms;
  const Label kStartLabel;
  const Label kEndLabel;
  std::set<Label>* exclude_labels;
  explicit
  CreateNgramTrieOptions(const size_t ngram_order,
                         const fst::SymbolTable& syms,
                         const Label& start_label, const Label& end_label)
      : kNgramOrder(ngram_order), kSyms(syms),
        kStartLabel(start_label), kEndLabel(end_label), exclude_labels(NULL)
  {}
};

/**
 * @brief Creates a trie that contains all ngrams up to a certain
 * order (e.g. uni-, bi- and trigrams for order 3).
 */
template<class Arc>
void CreateNgramTrie(fst::MutableFst<Arc>* ofst,
                     const CreateNgramTrieOptions<Arc>& opts) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;
  StateId start = ofst->AddState();
  ofst->SetStart(start);
  std::queue<StateId> states_queue;
  states_queue.push(start);
  for (int n = 0; n < opts.kNgramOrder; ++n) {
    int num_states = states_queue.size();
    while (--num_states >= 0) {
      StateId s = states_queue.front();
      states_queue.pop();
      SymbolTableIterator symIt(opts.kSyms);
      symIt.Next(); // ignore eps
      for (; !symIt.Done(); symIt.Next()) {
        Label label = symIt.Value();
        bool is_exclude_label = opts.exclude_labels != NULL
	  && opts.exclude_labels->find(label) != opts.exclude_labels->end();
        if (is_exclude_label || (s != start && label == opts.kStartLabel)) {
          continue;
        }
        StateId nextstate = ofst->AddState();
        ofst->SetFinal(nextstate, Weight::One());
        ofst->AddArc(s, Arc(label, label,
			    Weight::One(), nextstate));
        if (label != opts.kEndLabel) {
          states_queue.push(nextstate);
        }
      }
    }
  }
}

} } // end namespace fstrain::create

#endif
