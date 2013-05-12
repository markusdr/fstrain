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

#ifndef FSTRAIN_CREATE_NGRAM_COUNTER_H
#define FSTRAIN_CREATE_NGRAM_COUNTER_H

#include <iostream>
#include <cassert>

#include "fst/arcsort.h"
#include "fst/compose.h"
#include "fst/connect.h"
#include "fst/determinize.h"
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/matcher.h"
#include "fst/mutable-fst.h"
#include "fst/project.h"
#include "fst/push.h"
#include "fst/rmepsilon.h"
#include "fst/symbol-table.h"
#include "fst/union.h"
#include "fst/vector-fst.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/util.h" 
#include "fstrain/create/debug.h" 
#include "fstrain/util/determinized-union.h"
#include "fstrain/util/string-to-fst.h"
// #include "fstrain/util/print-fst.h"
// #include "fstrain/util/misc.h" // IsTrie
// #include "fstrain/create/add-fst-to-trie.h" // incorrect

namespace fstrain { namespace create {

/**
 * @brief Counts ngrams in FSTs.
 */
template<class Arc>
class NgramCounter {
    
 public:

  explicit NgramCounter(int ngram_order);

  ~NgramCounter();
    
  /**
   * @brief Adds the counts of ngrams contained in the FST
   */
  void AddCounts(const fst::Fst<Arc>& fst);

  /**
   * @brief Writes all counts as a trie into the result FST
   */
  void GetResult(fst::MutableFst<Arc>* result);

  /**
   * @brief Sets all counts to zero
   */
  void Reset();

 private:

  const int64 kSigmaLabel_;
  fst::MutableFst<Arc>* trie_;
  fst::MutableFst<Arc>* count_fst_;

};


namespace nsNgramCounterUtil {

/**
 * @brief Creates an FST that counts ngrams.  
 *
 * @see "The design principles and algorithms of a weighted grammar
 * library", Cyril Allauzen, Mehryar Mohri, Brian Roark,
 * Int. J. Found. Comput. Sci., vol. 16 (2005), pp. 403-421.
 */
template<class Arc>
void CreateNgramCountFst(int ngram_order, const int64 kSigmaLabel, 
                         fst::MutableFst<Arc>* ofst) {
  typedef typename Arc::StateId StateId;
  const int64 kEpsilonLabel = 0;
  StateId s = ofst->AddState();
  ofst->SetStart(s);
  ofst->AddArc(0, Arc(kSigmaLabel, kEpsilonLabel, Arc::Weight::One(), 0));
  StateId prev_state = s;  
  for(int i = 1; i <= ngram_order; ++i) {
    s = ofst->AddState();
    ofst->AddArc(prev_state, 
                 Arc(kSigmaLabel, kSigmaLabel, Arc::Weight::One(), s));
    if(i != ngram_order){
      ofst->AddArc(s, 
                   Arc(kEpsilonLabel, kEpsilonLabel, Arc::Weight::One(), ngram_order));
    }
    prev_state = s;
  }
  assert(ngram_order == s); // for those forward eps arcs
  ofst->AddArc(s, Arc(kSigmaLabel, kEpsilonLabel, Arc::Weight::One(), s));
  ofst->SetFinal(s, Arc::Weight::One());
  fst::ArcSort(ofst, fst::OLabelCompare<Arc>());
}

/**
 * @brief Composes the passed FST with the ngram count FST (using
 * sigma matcher).
 */
template<class Arc>
void CountNgrams(const fst::Fst<Arc>& fst,
                 const fst::Fst<Arc>& ngram_count_fst,
                 const int64 kSigmaLabel,
                 fst::MutableFst<Arc>* result) {
  using namespace fst;
  typedef Fst<Arc> FST;
  typedef SigmaMatcher< SortedMatcher<FST> > SM;
  ComposeFstOptions<Arc, SM> opts;
  const bool rewrite_both = true;
  opts.gc_limit = 0;
  opts.matcher1 = new SM(fst, MATCH_NONE, kNoLabel);
  opts.matcher2 = new SM(ngram_count_fst, MATCH_INPUT, kSigmaLabel, rewrite_both);
  ComposeFst<Arc> cfst(fst, ngram_count_fst, opts);
  Connect(result);
  *result = DeterminizeFst<Arc>(RmEpsilonFst<Arc>(ProjectFst<Arc>(cfst, PROJECT_OUTPUT)));
}

/**
 * @brief Pushes the weights from the final states to the arcs that
 * point to them.
 * 
 * Example: count('a')=6, count('ab')=3.
 *
 * Example input:
 * 0 1 a 1.0
 * 1     6.0
 * 1 2 b 1.0
 * 2     3.0
 *
 * Corresponding output: 
 * 0 1 a 6.0
 * 1 
 * 1 2 b 3.0
 * 2 
 */
template<class Arc>
void DistributeWeights(fst::MutableFst<Arc>* fst) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  for(StateIterator< MutableFst<Arc> > siter(*fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    for(MutableArcIterator< MutableFst<Arc> > aiter(fst, s); 
        !aiter.Done(); aiter.Next()){
      Arc arc = aiter.Value();
      arc.weight = fst->Final(arc.nextstate);
      aiter.SetValue(arc);
    }
  }
}

} // end namespace

template<class Arc>
NgramCounter<Arc>::NgramCounter(int ngram_order) 
    : kSigmaLabel_(-2), 
      trie_(new fst::VectorFst<Arc>()),
      count_fst_(new fst::VectorFst<Arc>())
{
  nsNgramCounterUtil::CreateNgramCountFst(ngram_order, kSigmaLabel_, count_fst_);  
}

template<class Arc>
NgramCounter<Arc>::~NgramCounter() {
  delete count_fst_;
  delete trie_;
}

template<class Arc>
void NgramCounter<Arc>::AddCounts(const fst::Fst<Arc>& fst) {
  fst::VectorFst<Arc> result;
  nsNgramCounterUtil::CountNgrams(fst, *count_fst_, kSigmaLabel_, &result);
  // AddFstToTrie(result, trie_); // TEST
  typedef typename fst::ILabelCompare<Arc> IComp;
  typedef typename fst::ArcSortFst<Arc, IComp> SortFst;
  fst::ArcSort(trie_, IComp());
  util::DeterminizedUnion<Arc>(trie_,
                               SortFst(result, IComp()));
}

template<class Arc>
void NgramCounter<Arc>::Reset() {
  delete trie_;
  trie_ = new fst::VectorFst<Arc>();
}

/**
 * @brief Puts all ngrams with counts in a trie.
 */
template<class Arc>
void NgramCounter<Arc>::GetResult(fst::MutableFst<Arc>* result) {
  *result = *trie_;
  fst::Push(result, fst::REWEIGHT_TO_FINAL);
  // nsNgramCounterUtil::DistributeWeights(result);
}

} } // end namespaces

#endif
