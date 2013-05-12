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
#ifndef FSTR_CREATE_ADD_BACKOFF_H
#define FSTR_CREATE_ADD_BACKOFF_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/matcher.h"
#include "fst/connect.h"
#include "fst/push.h"
#include "fst/arcsort.h"
#include "fstrain/create/debug.h"
#include "fstrain/util/misc.h" // HasOutArcs

namespace fstrain { namespace create {

template<class Arc>
void AddBackoff(fst::MutableFst<Arc>* fst, 
                typename Arc::StateId state, 
                typename Arc::StateId backoff_state, 
                int64 kPhiLabel) 
{
  using namespace fst;
  typedef typename Arc::StateId StateId;
  for(MutableArcIterator< MutableFst<Arc> > ait(fst, state); 
      !ait.Done(); ait.Next()) {
    StateId next_backoff_state;
    if(state == backoff_state) {
      next_backoff_state = backoff_state;
    }
    else {
      int64 label = ait.Value().ilabel;
      typedef fst::ILabelCompare<Arc> ICompare;
      // TODO: check that this does nothing but sort at requested
      // state
      ArcSortFst<Arc, ICompare> sorted(*fst, ICompare());
      Matcher< Fst<Arc> > matcher(sorted, MATCH_INPUT);
      matcher.SetState(backoff_state);
      bool found = matcher.Find(label);
      if(!found) {
        std::cerr << "Error at state " << state << ": Could not find label " << label 
                  << " at " << backoff_state 
                  << ". Is bigram '? " << label
                  << "' included, but not unigram '" << label << "'?" 
                  << std::endl;
      }
      assert(found);
      next_backoff_state = matcher.Value().nextstate;
    }    
    if(util::HasOutArcs(*fst, ait.Value().nextstate)) {
      AddBackoff(fst, ait.Value().nextstate, next_backoff_state, kPhiLabel);
    }
    else {
      Arc arc = ait.Value();
      arc.nextstate = next_backoff_state;
      ait.SetValue(arc);
    }
  }
  if(state != backoff_state) {
    fst->AddArc(state, 
                Arc(kPhiLabel, kPhiLabel, Arc::Weight::One(), backoff_state));
  }
}

template<class Arc>
void AddBackoff(fst::MutableFst<Arc>* fst, 
                const int64 kPhiLabel) 
{
  AddBackoff(fst, fst->Start(), fst->Start(), kPhiLabel);
}

/**
 * @brief Takes trie that encodes all ngrams and converts it to an
 * ngram model topology.  
 */
template<class Arc>
void ConvertTrieToModel(const int64 kStartLabel, 
                        const int64 kEndLabel, 
                        const int64 kPhiLabel, 
                        fst::MutableFst<Arc>* fst) {
  using namespace fst;
  typedef typename Arc::StateId StateId;

  // add backoff arcs and bend final arcs back
  AddBackoff(fst, kPhiLabel);

  // creates a new start state
  StateId delete_state = fst->AddState();
  for(MutableArcIterator< MutableFst<Arc> > ait(fst, fst->Start());
      !ait.Done(); ait.Next()) {
    if(ait.Value().ilabel == kStartLabel){
      Arc arc = ait.Value();
      StateId new_start_state = fst->AddState();        
      fst->AddArc(new_start_state, 
                    Arc(kStartLabel, kStartLabel, 
                        Arc::Weight::One(), arc.nextstate));
      fst->SetStart(new_start_state);
      arc.nextstate = delete_state;
      ait.SetValue(arc); // remove arc
      std::vector<StateId> delete_states_vec(1);
      delete_states_vec[0] = delete_state;
      fst->DeleteStates(delete_states_vec);
      break;
    }
  }

  // creates a new final state
  StateId final_state = fst->AddState();
  for(StateIterator<MutableFst<Arc> > sit(*fst);
      !sit.Done(); sit.Next()) {
    StateId s = sit.Value();
    fst->SetFinal(s, Arc::Weight::Zero());
    for(MutableArcIterator< MutableFst<Arc> > ait(fst, s);
        !ait.Done(); ait.Next()) {
      if(ait.Value().ilabel == kEndLabel){
        Arc arc = ait.Value();
        arc.nextstate = final_state;
        ait.SetValue(arc);
      }
    }    
  }
  fst->SetFinal(final_state, Arc::Weight::One()); 
  Connect(fst);
}

} } // end namespace fstrain::create


#endif
