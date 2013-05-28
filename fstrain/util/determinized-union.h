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
#ifndef FSTRAIN_UTIL_DETERMINIZED_UNION_H
#define FSTRAIN_UTIL_DETERMINIZED_UNION_H

#include <iostream>
#include <queue>
#include <set>
#include <utility> // std::pair

#include <boost/foreach.hpp>

#include "fst/fst.h"
#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fst/properties.h"
#include "fst/arcsort.h"

#include "fstrain/util/debug.h"
// #include "fstrain/util/print-fst.h"

namespace fstrain { namespace util { 

/**
 * @brief Adds fst2 to fst1 (union) and determinizes on the
 * fly. Conditions: fst2 must be a lattice and fst1 empty or a trie.
 *
 * This is useful when many lattices need to be unionized and the
 * result be a trie (much more efficient than repeatedly unionizing
 * and determinizing separately, especially since OpenFst does not
 * determinize in place).
 *
 * See Mehryar Mohri (2009), "Weighted Automata Algorithms", in:
 * Handbook of Weighted Automata, for how the weights are computed.
 *
 * See also fst/determinize.h and fst/union.h.
 */
template <class Arc>
class DeterminizedUnion {

  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  struct Element {
    Element() {}
    Element(StateId s, Weight w) : state_id(s), weight(w) {}
    StateId state_id;  // Input state Id
    Weight weight;     // Residual weight
  };

  typedef std::pair<Element,Element> Subset;  
  
  struct CompareSubsets {
    bool operator()(const Subset& s1, const Subset& s2) const {
      if (s1.first.state_id < s2.first.state_id) {
        return true;
      }
      else if (s2.first.state_id < s1.first.state_id) {
        return false;
      }
      else {
        if (s1.second.state_id < s2.second.state_id) {
          return true;
        }
        else if (s2.second.state_id < s1.second.state_id) {
          return false;
        }
        else {
          if (s1.first.weight.Value() < s2.first.weight.Value()) {
            return true;
          }
          else if (s2.first.weight.Value() < s1.first.weight.Value()) {
            return false;
          }
          else {
            if (s1.second.weight.Value() < s2.second.weight.Value()) {
              return true;
            }
          }
        }
      }
      return false;
    }
  };

 public:

  DeterminizedUnion(fst::MutableFst<Arc> *fst1, 
                    const fst::Fst<Arc>& fst2, 
                    double delta = 1e-8) 
      : delta_(delta) {
    using namespace fst;
    // CheckInput(*fst1, fst2);
    if (fst2.Start() == kNoStateId) {
      return;
    }
    if (fst1->Start() == kNoStateId) {
      StateId s = fst1->AddState();
      fst1->SetStart(s); 
    }
    Subset ss(Element(fst1->Start(), Weight::One()), 
              Element(fst2.Start(), Weight::One()));
    fst1->SetFinal(fst1->Start(), 
                   Plus(fst1->Final(fst1->Start()), 
                        fst2.Final(fst2.Start())));
    std::queue<Subset> the_queue;
    std::set<Subset, CompareSubsets> already_processed;
    the_queue.push(ss);

    while (!the_queue.empty()) {
      Subset q = the_queue.front();
      the_queue.pop();
      FSTR_UTIL_DBG_MSG(10, "Dequeued (" << q.first.state_id << "," 
                        << q.second.state_id << ")" << std::endl);
      std::vector<Arc> arcs_to_add;
      MutableArcIterator<MutableFst<Arc> > ait1(fst1, q.first.state_id);
      ArcIterator<Fst<Arc> > ait2(fst2, q.second.state_id);

      // based on standard union alg, see
      // http://www.cplusplus.com/reference/algorithm/set_union/
      while (true) {

        if (ait1.Done()) {
          while(!ait2.Done()) {
            arcs_to_add.push_back(ait2.Value());
            ait2.Next();
          }
          break;
        }
        if (ait2.Done()) {
          while(!ait1.Done()) {
            Arc arc = ait1.Value();
            arc.weight = Times(q.first.weight, arc.weight);
            ait1.SetValue(arc);
            ait1.Next();
          }
          break;
        }

        if (ait1.Value().ilabel < ait2.Value().ilabel) {
          Arc arc = ait1.Value();
          arc.weight = Times(q.first.weight, arc.weight);
          ait1.SetValue(arc);
          ait1.Next();
        }
        else if (ait2.Value().ilabel < ait1.Value().ilabel) {
          arcs_to_add.push_back(ait2.Value());
          ait2.Next();
        }
        else {
          Arc new_arc = ait1.Value();
          Weight vw1 = Times(q.first.weight, ait1.Value().weight);
          Weight vw2 = Times(q.second.weight, ait2.Value().weight);
          new_arc.weight = Plus(vw1, vw2);
          ait1.SetValue(new_arc);
          Weight v1 = Divide(vw1, new_arc.weight).Quantize(delta_);
          Weight v2 = Divide(vw2, new_arc.weight).Quantize(delta_);
          Subset ss(Element(ait1.Value().nextstate, v1), 
                    Element(ait2.Value().nextstate, v2));
          if(already_processed.find(ss) == already_processed.end()) {
            the_queue.push(ss); 
            already_processed.insert(ss);
            Weight vr1 = Times(v1, fst1->Final(ait1.Value().nextstate));
            Weight vr2 = Times(v2, fst2.Final(ait2.Value().nextstate));
            fst1->SetFinal(ait1.Value().nextstate, Plus(vr1, vr2));
          }
          else {
            std::cerr << "Error: encountering same subset ("<<ss.first.state_id<<","<<ss.second.state_id<<") again." << std::endl
                      << "should not happen if input is a trie and a lattice" 
                      << std::endl;
            exit(1);
          } 
          ait1.Next();
          ait2.Next();
        }

      } // end while

      BOOST_FOREACH (Arc arc, arcs_to_add) {
        StateId s1 = fst1->AddState();
        StateId s2 = arc.nextstate;
        arc.nextstate = s1;
        Weight vw2 = Times(q.second.weight, arc.weight);
        arc.weight = vw2;
        fst1->AddArc(q.first.state_id, arc);
        Weight v1 = Divide(Weight::One(), arc.weight).Quantize(delta_);
        Weight v2 = Divide(vw2, arc.weight).Quantize(delta_);
        Subset ss(Element(s1, v1), Element(s2, v2));
        // ss has never been processed (since s1 is a new state)
        the_queue.push(ss);   
        already_processed.insert(ss);
        Weight vr1 = Times(v1, fst1->Final(s1)); // zero
        Weight vr2 = Times(v2, fst2.Final(s2));
        fst1->SetFinal(s1, Plus(vr1, vr2));
      }
//      if (!arcs_to_add.empty()) {
//        SortArcs(fst1, q.first.state_id, ILabelCompare<Arc>());
//      }

//      // TEST
//      std::set<Label> labels;
//      for (fst::ArcIterator< fst::MutableFst<Arc> > aiter(*fst1, q.first.state_id);
//           !aiter.Done(); aiter.Next()) {
//        if (labels.find(aiter.Value().ilabel) != labels.end()) {
//          std::cerr << "State " << q.first.state_id << " now non-deterministic";
//          exit(1);
//        }
//        labels.insert(aiter.Value().ilabel);
//      }

            
    }

  } // end constructor

 private:

  void CheckInput(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2) {
    using namespace fst;
    if (!fst1.Properties(kAcceptor, true)) {
      FSTR_UTIL_EXCEPTION("fst1 must be an acceptor");
    }
    if (!fst2.Properties(kAcceptor, true)) {
      FSTR_UTIL_EXCEPTION("fst2 must be an acceptor");
    }
    if (!fst1.Properties(kIDeterministic, true)) {
      FSTR_UTIL_EXCEPTION("fst1 must be deterministic");
    }
    if (!fst2.Properties(kIDeterministic, true)) {
      FSTR_UTIL_EXCEPTION("fst2 must be deterministic");
    }
    if (!fst1.Properties(kILabelSorted, true)) {
      FSTR_UTIL_EXCEPTION("fst1 must be input-label sorted");
    }
    if (!fst2.Properties(kILabelSorted, true)) {
      FSTR_UTIL_EXCEPTION("fst2 must be input-label sorted");
    }
  }

  template<class Compare>
  void SortArcs(fst::MutableFst<Arc>* fst, StateId s, 
                Compare comp) {
    std::vector<Arc> arcs;
    arcs.clear();
    for (fst::ArcIterator< fst::MutableFst<Arc> > aiter(*fst, s);
         !aiter.Done(); aiter.Next())
      arcs.push_back(aiter.Value());
    sort(arcs.begin(), arcs.end(), comp);
    fst->DeleteArcs(s);
    BOOST_FOREACH (const Arc& arc, arcs) {
      fst->AddArc(s, arc);    
    }
  }

  double delta_;


}; // end class

} } // end namespace

#endif
