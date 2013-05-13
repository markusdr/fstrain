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
// Author: jrs026@gmail.com (Jason Smith)
//
#ifndef _GETWELLFORMED_H_
#define _GETWELLFORMED_H_

#include "fst/symbol-table.h"
#include "fst/mutable-fst.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace create {

// Basic setup for every WF machine
template<class Arc>
void initWFMachine(fst::MutableFst<Arc> *m,
		   const fst::SymbolTable* syms)
{
  m->DeleteStates();
  m->AddState();
  m->SetStart(0);
  m->SetFinal(0, Arc::Weight::One());
  m->SetInputSymbols(syms);
  m->SetOutputSymbols(syms);
}


// Create a machine that accepts any string
void getDummyMachine(fst::MutableFst<fst::StdArc> *dummy,
		     const fst::SymbolTable* syms);


// Creates a machine that limits the number of insertions
template<class Arc>
void getLimitMachine(fst::MutableFst<Arc> *limitMachine,
		     const fst::SymbolTable* syms, int limit, bool alsoRestrictDel = false)
{
  typedef typename Arc::Weight Weight;
  initWFMachine(limitMachine, syms);

  // Create all needed states
  for (int l = 1; l <= limit; l++) {
    limitMachine->AddState();
    limitMachine->SetFinal(l, Weight::One());
  }

  fst::SymbolTableIterator i(*syms);
  i.Next(); // skip epsilon
  while(!i.Done()) {
    if (i.Symbol()[0] == '-' || (alsoRestrictDel && i.Symbol()[2] == '-' )) { // Insertion (or deletion)
      for (int j = 0; j < limit; j++) {
        limitMachine->AddArc(j,
                             Arc(i.Value(), i.Value(), Weight::One(), j + 1));
      }
    }
    else {
      for (int j = 0; j <= limit; j++) {
        limitMachine->AddArc(j,
                             Arc(i.Value(), i.Value(), Weight::One(), 0));
      }
    }

    i.Next();
  }
}


// Create a machine that accepts strings of the same conjugation
void getConjMachine(fst::MutableFst<fst::StdArc> *conjMachine,
		    const fst::SymbolTable* syms, int conjs);


// Create a machine that accepts well formed change segment strings
void getChgSegMachine(fst::MutableFst<fst::StdArc> *chgSegMachine,
		      const fst::SymbolTable* syms, int segs);

/**
 * Builds a one-state machine that transduces from alignment symbols to annotated alignment symbols.
 * It is needed to connect the given n-best input/output alignments (e.g. S|S j|j u|u m|m p|p -|e -|d)
 * to the wellformedness machine (which has symbols like j|j-c2-g1))
 */
void getAnnotateMachine(const fst::SymbolTable* ax,         // a|x
			const fst::SymbolTable* ax_la,       // a|x-c1-g2
			fst::MutableFst<fst::StdArc>* result);   // a|x : a|x-c1-g2


void getAnnotateMachine_old(fst::MutableFst<fst::StdArc>* annotateAlignmentSyms, // a|x : a|x-c2-g1
                            const fst::SymbolTable* annotatedSyms);        // a|x-c2-g1


///**
// * Reads alignmentConstraintFile 
// * (which typically contains all n-best alignments of training data, e.g. S|S a|x b|- c|y -|z E|E etc)
// * and creates a machine that only allows such symbols (with annotations attached, e.g. a|x-c1-g2)
// */
//void getConstrainSymbolsMachine(fst::MutableFst<fst::StdArc>* constrainSymbolsMachine, 
//				const fst::SymbolTable* syms, // e.g., a|x-c2-g1
//				const char* alignmentConstraintFile);


// Create a machine that only accepts well formed alignment strings
void getWellFormed(
    fst::MutableFst<fst::StdArc>* wellFormed,
    const fst::SymbolTable* syms,
    int addLimit = 4,
    int conjs = 0, int segs = 0, const char* alignmentConstraintFile = 0, bool alsoRestrictDel = false);

} } // end namespaces

#endif

