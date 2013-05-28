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
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include "fst/arcsort.h"
#include "fst/compose.h"
#include "fst/concat.h"
#include "fst/intersect.h"
#include "fst/rmepsilon.h"
#include "fstrain/util/determinize-in-place.h"
#include "fst/minimize.h"
#include "fst/connect.h"
#include "fst/project.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/debug.h"
#include "fstrain/util/print-fst.h"
// #include "fstrain/create/fst-util.h"
#include "fstrain/create/wfutil.h"
#include "fstrain/create/get-well-formed.h"

namespace fstrain { namespace create {

using fst::Fst;
using fst::MutableFst;
using fst::VectorFst;
using fst::SymbolTable;
using fst::SymbolTableIterator;
using wfutil::getLA;
typedef fst::StdArc Arc;
typedef Arc::Weight Weight;

// Create a machine that accepts any string
void getDummyMachine(MutableFst<Arc> *dummy,
                     const SymbolTable* syms)
{
  initWellformedMachine(dummy, syms);
  SymbolTableIterator i(*syms);
  i.Next(); // skip epsilon
  for (; !i.Done(); i.Next())
  {
    const std::string symbol = i.Symbol();
    //if (symbol != "S|S" && symbol != "E|E") {
    dummy->AddArc(0,
                  Arc(i.Value(), i.Value(), Weight::One(), 0));
    //}
  }
}

// Create a machine that accepts strings of the same conjugation
void getConjMachine(MutableFst<Arc> *conjMachine,
                    const SymbolTable* syms, int conjs)
{
  initWellformedMachine(conjMachine, syms);

  // Iterate over each conjugation's state
  for (int c = 1; c <= conjs; c++) {
    // Add a state for this conjugation
    conjMachine->AddState();
    conjMachine->SetFinal(c, Weight::One());
    // Create transition to this state from the start
    conjMachine->AddArc(0,
                        Arc(0, 0, Weight::One(), c));
  }

  // Create self loops for all arcs of the correct conjugation
  SymbolTableIterator i(*syms);
  i.Next(); // skip epsilon
  while (!i.Done())
  {
    const std::string symbol = i.Symbol();
    int currentConj = getLA(i.Symbol(), "-c");
    conjMachine->AddArc(currentConj,
                        Arc(i.Value(), i.Value(), Weight::One(), currentConj));
    i.Next();
  }
}


// change regions machine: changes have odd nums, identities have even
// nums
void getChgSegMachine3(MutableFst<Arc> *chgSegMachine,
                       const SymbolTable* syms, int segs)
{
  // initWellformedMachine(chgSegMachine, syms);
  int num_states = segs * 2 + 2;
  for (int i = 0; i < num_states; ++i) {
    chgSegMachine->AddState();
  }
  chgSegMachine->SetStart(0);
  chgSegMachine->SetInputSymbols(syms);
  chgSegMachine->SetOutputSymbols(syms);
  for (int i = 1; i < num_states; ++i) {
    chgSegMachine->AddArc(0, Arc(0, 0, Weight::One(), i));
    chgSegMachine->SetFinal(i, Weight::One());
  }

  SymbolTableIterator it(*syms);
  it.Next(); // ignore eps
  for (; !it.Done(); it.Next()) {
    const std::string symbol = it.Symbol();
    FSTR_CREATE_DBG_MSG(10, "Symbol " + symbol << std::endl);
    if (symbol == "S|S" || symbol == "E|E") {
      continue;
    }
    int currentRegion = getLA(symbol, "-g");
    bool isChangeSymbol = (symbol[0] != symbol[2]);
    if (isChangeSymbol) {
      assert(currentRegion % 2 != 0); // odd
      for (int fromState=1; fromState < currentRegion + 1; fromState+=2) {
        chgSegMachine->AddArc(fromState, Arc(it.Value(), it.Value(), Weight::One(), currentRegion+1));
      }
      chgSegMachine->AddArc(currentRegion+1, Arc(it.Value(), it.Value(), Weight::One(), currentRegion+1)); // self-loop
    }
    else { // not a change sym (identity)
      assert(currentRegion % 2 == 0); // even
      for (int fromState = 2; fromState < currentRegion + 1; fromState+=2) {
        chgSegMachine->AddArc(fromState, Arc(it.Value(), it.Value(), Weight::One(), currentRegion+1));
      }
      chgSegMachine->AddArc(currentRegion+1, Arc(it.Value(), it.Value(), Weight::One(), currentRegion+1)); // self-loop
    }
  }
}

// Create a machine that has only 2 states connected by one arc
void getOneArcFst(const Arc::Label label, MutableFst<Arc>* result) {
  Arc::StateId s1 = result->AddState();
  Arc::StateId s2 = result->AddState();
  result->SetStart(s1);
  result->SetFinal(s2, Weight::One());
  result->AddArc(s1,
                 Arc(label, label, Weight::One(), s2));
}

// Create a machine that only accepts well formed alignment strings
void getWellFormed(
    MutableFst<Arc>* wellFormed,
    const SymbolTable* syms,
    int addLimit,
    int conjs, int segs, const char* alignmentConstraintFile, bool alsoRestrictDel)
{
  FSTR_CREATE_DBG_MSG(10, "Wellformed, limit=" << addLimit << ", conjs=" << conjs
                      << ", " << segs << ", syms=" << syms << std::endl);
  std::vector<MutableFst<Arc>* > machines;
  machines.clear();
  // Make a machine that accepts anything
  MutableFst<Arc>* dummy = new VectorFst<Arc>();
  getDummyMachine(dummy, syms);

  // Make a machine to limit the number of insertions
  MutableFst<Arc>* limitMachine = new VectorFst<Arc>();
  if (addLimit >= 0)
  {
    getLimitMachine(limitMachine, syms, addLimit, alsoRestrictDel);
    FSTR_CREATE_DBG_EXEC(10, std::cerr << "Limit machine:" << std::endl;
                         fstrain::util::printTransducer(limitMachine,
                                                        limitMachine->InputSymbols(), limitMachine->OutputSymbols(), std::cerr););
  }
  else {
    getDummyMachine(limitMachine, syms);
  }
  machines.push_back(limitMachine);

  // Make a machine for conjugations
  if (conjs > 0)
  {
    MutableFst<Arc>* conjMachine = new VectorFst<Arc>();
    getConjMachine(conjMachine, syms, conjs);
    machines.push_back(conjMachine);
    FSTR_CREATE_DBG_EXEC(10,
                         fstrain::util::printTransducer(conjMachine,
                                                        conjMachine->InputSymbols(),
                                                        conjMachine->OutputSymbols(),
                                                        std::cerr));
  }

  // Make a machine for change segments
  if (segs > 0) {
    MutableFst<Arc>* chgSegMachine = new VectorFst<Arc>();
    getChgSegMachine3(chgSegMachine, syms, segs);
    machines.push_back(chgSegMachine);
    FSTR_CREATE_DBG_EXEC(10, std::cerr << "Change FST:" << std::endl;
                         fstrain::util::printTransducer(
                             chgSegMachine,
                             chgSegMachine->InputSymbols(),
                             chgSegMachine->OutputSymbols(), std::cerr););
  }

  // Intersect all the machines and store it in wellFormed
  std::vector<MutableFst<Arc>* >::iterator it;
  for (it = machines.begin(); it != machines.end(); it++) {
    fst::ArcSort(dummy, fst::StdOLabelCompare());
    fst::Intersect(*dummy, **it, wellFormed);
    // delete dummy->InputSymbols();
    // delete dummy->OutputSymbols();
    delete dummy;
    delete *it;
    dummy = wellFormed->Copy();
  }
  delete dummy;

  VectorFst<Arc> start_fst;
  const Arc::Label kStartLabel = syms->Find("S|S");
  getOneArcFst(kStartLabel, &start_fst);

  VectorFst<Arc> end_fst;
  const Arc::Label kEndLabel = syms->Find("E|E");
  getOneArcFst(kEndLabel, &end_fst);


  start_fst.SetInputSymbols(syms);
  start_fst.SetOutputSymbols(syms);
  end_fst.SetInputSymbols(syms);
  end_fst.SetOutputSymbols(syms);
  Concat(start_fst, wellFormed);
  end_fst.SetInputSymbols(wellFormed->OutputSymbols());
  Concat(wellFormed, end_fst);

  RmEpsilon(wellFormed);
  Connect(wellFormed);
  fstrain::util::Determinize(wellFormed);
  Minimize(wellFormed);
}

} } // end namespaces
