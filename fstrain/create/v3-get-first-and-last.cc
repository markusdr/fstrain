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

namespace fstrain { namespace create { namespace v3 {

using fst::MutableFst;
using fst::SymbolTable;
using fst::SymbolTableIterator;
using wfutil::toString;
typedef fst::StdArc Arc;
typedef Arc::Weight Weight;

void CreateAlignmentSymbolTable(const fst::SymbolTable& isymbols,
                                const fst::SymbolTable& osymbols,
                                fst::SymbolTable* result) {
  result->AddSymbol("eps");
  for(fst::SymbolTableIterator iit(isymbols); !iit.Done(); iit.Next()) {
    const bool in_is_eps = iit.Value() == 0;
    const std::string isym = in_is_eps ? "-" : iit.Symbol();
    for(fst::SymbolTableIterator oit(osymbols); !oit.Done(); oit.Next()) {
      const bool out_is_eps = oit.Value() == 0;
      if(in_is_eps && out_is_eps) {
        continue;
      }
      const std::string osym = out_is_eps ? "-" : oit.Symbol();
      if((isym == "S" && osym != "S") || (isym == "E" && osym != "E")
         || (osym == "S" && isym != "S") || (osym == "E" && isym != "E")) {
        continue;
      }
      std::string align_sym = isym + "|" + osym;
      result->AddSymbol(align_sym);
    }
  }
}

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

// Creates transducers to go to and from the alignment characters
void GetFirstAndLast(const fst::SymbolTable* iSyms,
                     const fst::SymbolTable* oSyms,
                     fst::MutableFst<fst::StdArc>* projUp,
                     fst::MutableFst<fst::StdArc>* projDown,
                     int conjs = 0, int segs = 0, int types = 0) {

  const char sepChar = '|';

  // Alignment character symbol table
  SymbolTable *alSyms = new SymbolTable("align-syms");
  CreateAlignmentSymbolTable(*iSyms, *oSyms, alSyms);

  // Add latent annotations
  SymbolTable* resultOsyms = NULL;
  SymbolTable* resultOsyms2 = NULL;
  SymbolTable* resultOsyms3 = NULL;
  if (conjs > 0) {
    resultOsyms = new SymbolTable("");
    getConjSymbolTable(*alSyms, conjs, resultOsyms);
  }
  else {
    resultOsyms = alSyms;
  }
  if (segs > 0) {
    resultOsyms2 = new SymbolTable("");
    getChangeSegSymbolTable(*resultOsyms, segs, resultOsyms2);
    delete resultOsyms;
  }
  else {
    resultOsyms2 = resultOsyms;
  }
  if (types > 0) {
    resultOsyms3 = new SymbolTable("");
    getLetterTypeSymbolTable(*resultOsyms2, types, resultOsyms3);
    delete resultOsyms2;
  }
  else {
    resultOsyms3 = resultOsyms2;
  }

  if(conjs > 0 || segs > 0 || types > 0) {
    delete alSyms;
  }
  alSyms = resultOsyms3;

  // Create single state machines with these symbols,
  // one for projecting up and one for projecting down.
  initHookMachine(projUp, iSyms, alSyms);
  initHookMachine(projDown, alSyms, oSyms);

  SymbolTableIterator sit(*alSyms);
  sit.Next(); // skip epsilon
  std::string up, al, down;
  while (!sit.Done())
  {
    al = sit.Symbol();

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

    sit.Next();
  }
//  if(conjs > 0 || segs > 0 || types > 0){
//    delete alSyms;
//  }
  delete alSyms; // can always delete b/c it was copied into the FST
}

} } } // end namespaces

