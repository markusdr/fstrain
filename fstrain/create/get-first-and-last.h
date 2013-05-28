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
#ifndef FSTRAIN_CREATE_GETFIRSTANDLAST_H_
#define FSTRAIN_CREATE_GETFIRSTANDLAST_H_

#include <set>
#include "fst/symbol-table.h"
#include "fst/mutable-fst.h"
#include "fst/arc.h"

namespace fstrain { namespace create {


/** Constructs a basic symbol table with all possible
	alignment characters.  May be replaced later on by
	recieving possible alignment characters as input.
*/
void getBasicSymbolTable(const std::set<char>& inputLetters,
			 const std::set<char>& outputLetters,
			 fst::SymbolTable* syms);

/** Adds conjugation labels to the symbol table, where
	the number of conjugations is given by conjs.
*/
void getConjSymbolTable(fst::SymbolTable* syms, int conjs);


/** Adds change segment labels to the symbol table, where
	the number of segments is given by segs.
*/
void getChangeSegSymbolTable(fst::SymbolTable* syms, int segs);


/** Adds letter labels to the symbol table, where the number
	of labels is given by types.
*/
void getLetterTypeSymbolTable(fst::SymbolTable* syms, int types);

void initHookMachine(fst::MutableFst<fst::StdArc>* f, const fst::SymbolTable* iSyms, const fst::SymbolTable* oSyms);

// Creates transducers to go to and from the alignment characters
void getFirstAndLast(
	fst::MutableFst<fst::StdArc>* projUp,
	fst::MutableFst<fst::StdArc>* projDown,
	const std::set<char>& inputSyms,
	const std::set<char>& outputSyms,
	const fst::SymbolTable* givenBasicAlignmentSyms,
	int conjs = 0, int segs = 0, int types = 0);

} } // end namespaces

#endif
