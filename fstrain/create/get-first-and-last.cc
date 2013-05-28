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
#include <set>
#include <sstream>
#include "fstrain/create/wfutil.h"
#include "fst/symbol-table.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fstrain/create/debug.h"

namespace fstrain { namespace create {

using fst::MutableFst;
using fst::SymbolTable;
using fst::SymbolTableIterator;
using wfutil::toString;
typedef fst::StdArc Arc;
typedef Arc::Weight Weight;

/** Adds conjugation labels to the symbol table, where
    the number of conjugations is given by conjs.
*/
void getConjSymbolTable(const SymbolTable& syms, int conjs, SymbolTable* result)
{
  // Add epsilon
  result->AddSymbol("eps");

  SymbolTableIterator i(syms);
  i.Next(); // skip epsilon
  std::string alc, new_alc;
  for (; !i.Done(); i.Next())
  {
    alc = i.Symbol();
    if(alc == "S|S" || alc == "E|E"){
      result->AddSymbol(alc);
    }
    else {
      alc += "-c";
      for (int j = 0; j < conjs; j++)
      {
        new_alc = alc + toString(j+1);
        result->AddSymbol(new_alc.c_str());
      }
    }
  }
}

/** Adds change segment labels to the symbol table, where
    the number of segments is given by segs.
*/
void getChangeSegSymbolTable(const SymbolTable& syms, int segs, SymbolTable* result)
{
  const char sepChar = '|';

  // Add epsilon
  result->AddSymbol("eps");

  SymbolTableIterator *i = new SymbolTableIterator(syms);
  i->Next(); // skip epsilon
  int j;
  std::string alc, new_alc;
  while (!i->Done())
  {
    alc = i->Symbol();

    std::string::size_type sepPos = alc.find(sepChar);
    if(sepPos == std::string::npos || alc.length() == sepPos + 1){
      FSTR_CREATE_EXCEPTION("Did not find separator or output side in " << alc);
    }
    std::string up = alc.substr(0,sepPos);
    std::string down = alc.substr(sepPos+1);

    // This test assumes alignments of single characters
    // as symbols
    if (up == down)// Identity character
    {
      if(up == "S"){ // start symbol
        // alc += "-g0";
        result->AddSymbol(alc.c_str());	// S|S-g0
      }
      else if(up == "E"){ // end symbol
        // alc += "-g" + toString(segs * 2);   // E|E-g6
        result->AddSymbol(alc.c_str());
      }
      else{
        alc += "-g";
        for(int j=0; j<=segs*2; j+=2){
          new_alc = alc + toString(j);      // a|a-0, a|a-2, a|a-4, a|a-6
          result->AddSymbol(new_alc.c_str());
        }
      }
    }
    else // Not an identity character
    {
      alc += "-g";
      for (j = 1; j < segs*2; j+=2)
      {
        new_alc = alc + toString(j); // a|x-1, a|x-3, a|x-5
        result->AddSymbol(new_alc.c_str());
      }
    }
    i->Next();
  }
  delete i;
}

/** Adds letter labels to the symbol table, where the number
    of labels is given by types.
*/
void getLetterTypeSymbolTable(const SymbolTable& syms, int types, SymbolTable* result)
{
  // Add epsilon
  result->AddSymbol("eps");

  SymbolTableIterator *i = new SymbolTableIterator(syms);
  i->Next(); // skip epsilon
  int j, k;
  std::string alc, new_alc;
  while (!i->Done())
  {
    alc = i->Symbol();
    for (j = 0; j < types; j++)
    {
      for (k = 0; k < types; k++)
      {
        new_alc = alc + "-i" + toString(j+1) + "-o" + toString(k+1);
        result->AddSymbol(new_alc.c_str());
      }
    }
    i->Next();
  }
  delete i;
}

void initHookMachine(MutableFst<Arc>* f, const SymbolTable* iSyms, const SymbolTable* oSyms){
  f->DeleteStates();
  f->AddState();
  f->AddState();
  f->AddState();
  f->SetStart(0);
  f->SetFinal(2, Weight::One());
  f->SetInputSymbols(iSyms);
  f->SetOutputSymbols(oSyms);
}

// Create transducers to go to and from the alignment characters
void getFirstAndLast(
    MutableFst<Arc>* projUp,
    MutableFst<Arc>* projDown,
    const std::set<char>& inputSyms,
    const std::set<char>& outputSyms,
    const SymbolTable* givenBasicAlignmentSyms,
    int conjs, int segs, int types)
{
  const char sepChar = '|';

  // Alignment character symbol table
  SymbolTable *alSyms = 0;

  // Generate the basic symbol table with all possible symbols (e.g. a|x etc) ...
  if(givenBasicAlignmentSyms == 0){
    FSTR_CREATE_EXCEPTION("need basic alignment symbol table");
  }
  else{
    alSyms = const_cast<SymbolTable*>(givenBasicAlignmentSyms);
  }

  // Create the input and output character symbol tables
  SymbolTable *iSyms = new SymbolTable("input-letters");
  SymbolTable *oSyms = new SymbolTable("output-letters");
  // Add epsilons as the first symbols
  iSyms->AddSymbol("eps");
  oSyms->AddSymbol("eps");

  SymbolTableIterator* i = new SymbolTableIterator(*alSyms);
  i->Next(); // skip epsilon
  while (!i->Done())
  {
    std::string tempSym = i->Symbol(); // e.g. in|out
    std::string::size_type sepPos = tempSym.find(sepChar);
    if(sepPos == std::string::npos || tempSym.length() == sepPos + 1){
      FSTR_CREATE_EXCEPTION("Did not find separator or output side in " << tempSym);
    }
    iSyms->AddSymbol(tempSym.substr(0,sepPos)); // in
    oSyms->AddSymbol(tempSym.substr(sepPos+1)); // out
    i->Next();
  }
  delete i;

  // Add latent annotations
  SymbolTable* resultOsyms = NULL;  // this fixes a segmentation fault that I got. TODO: there are still memory leaks
  SymbolTable* resultOsyms2 = NULL;
  SymbolTable* resultOsyms3 = NULL;
  if (conjs > 0)
  {
    resultOsyms = new SymbolTable("");
    getConjSymbolTable(*alSyms, conjs, resultOsyms);
  }
  else{
    resultOsyms = alSyms;
  }
  if (segs > 0)
  {
    resultOsyms2 = new SymbolTable("");
    getChangeSegSymbolTable(*resultOsyms, segs, resultOsyms2);
    delete resultOsyms;
  }
  else{
    resultOsyms2 = resultOsyms;
  }
  if (types > 0)
  {
    resultOsyms3 = new SymbolTable("");
    getLetterTypeSymbolTable(*resultOsyms2, types, resultOsyms3);
    delete resultOsyms2;
  }
  else{
    resultOsyms3 = resultOsyms2;
  }

  if(givenBasicAlignmentSyms == 0 && (conjs > 0 || segs > 0 || types > 0)){
    delete alSyms;
  }
  alSyms = resultOsyms3;

  // Create single state machines with these symbols,
  // one for projecting up and one for projecting down.
  initHookMachine(projUp, iSyms, alSyms);
  initHookMachine(projDown, alSyms, oSyms);

  i = new SymbolTableIterator(*alSyms);
  i->Next(); // skip epsilon
  std::string up, al, down;
  while (!i->Done())
  {
    al = i->Symbol();

    std::string::size_type sepPos = al.find(sepChar);
    if(sepPos == std::string::npos || al.length() == sepPos + 1){
      FSTR_CREATE_EXCEPTION("Did not find separator or output side in " << al);
    }
    up = al.substr(0,sepPos);
    if(up == "-"){
      up = "eps";
    }
    down = al.substr(sepPos+1);  // a|x-c1 => x-c1, OR a|--c1 => --c1
    if(down[0] == '-'){
      down = "eps";
    }
    else {
      std::string::size_type hyphenpos = down.find('-');
      if(hyphenpos != std::string::npos){
        down = down.substr(0, hyphenpos);
      }
    }

    bool isStartSymbol = up == "S" && down == "S";
    bool isEndSymbol = up == "E" && down == "E";
    int sourceState = isStartSymbol ? 0 : isEndSymbol ? 1 : 1;
    int targetState = isStartSymbol ? 1 : isEndSymbol ? 2 : 1;

    projUp->AddArc(sourceState, Arc(
                       iSyms->Find(up), // Input symbol
                       alSyms->Find(al), // Output Symbol
                       Weight::One(), // Cost
                       targetState
                       ));

    projDown->AddArc(sourceState, Arc(
                         alSyms->Find(al), // Input Symbol
                         oSyms->Find(down), // Output symbol
                         Weight::One(), // Cost
                         targetState
                         ));

    i->Next();
  }
  delete i;
  delete iSyms;
  delete oSyms;
  if(conjs > 0 || segs > 0 || types > 0){
    delete alSyms;
  }
}

} } // end namespaces

