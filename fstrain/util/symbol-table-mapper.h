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

#ifndef FSTRAIN_UTIL_SYMBOL_TABLE_MAPPER_H
#define FSTRAIN_UTIL_SYMBOL_TABLE_MAPPER_H

#include "fst/map.h"
#include "fst/symbol-table.h"

namespace fstrain { namespace util {

using namespace fst;

/**
 * @brief Mapper that maps the symbols from one table to another
 */
template<class Arc>
struct SymbolTableMapper {

  const SymbolTable& inOldSyms;
  const SymbolTable& inNewSyms;

  const SymbolTable& outOldSyms;
  const SymbolTable& outNewSyms;

  explicit SymbolTableMapper(const SymbolTable& inOldSyms_,
                             const SymbolTable& inNewSyms_,
			     const SymbolTable& outOldSyms_,
                             const SymbolTable& outNewSyms_)
    : inOldSyms(inOldSyms_), inNewSyms(inNewSyms_),
      outOldSyms(outOldSyms_), outNewSyms(outNewSyms_)
  {}

  Arc operator()(const Arc &arc) const {
    using namespace fst;
    Arc new_arc = arc;
    const std::string& isym = inOldSyms.Find(arc.ilabel);
    const std::string& osym = outOldSyms.Find(arc.olabel);
    new_arc.ilabel = inNewSyms.Find(isym);
    new_arc.olabel = outNewSyms.Find(osym);
    return new_arc;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_CLEAR_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_CLEAR_SYMBOLS;}

  uint64 Properties(uint64 props) const { return props; }

}; // end class


/**
 * @brief Mapper that removes the symbol table pointers
 */
template<class Arc>
struct RemoveSymbolTableMapper {

  Arc operator()(const Arc &arc) const {
    return arc;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_CLEAR_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_CLEAR_SYMBOLS;}

  uint64 Properties(uint64 props) const { return props; }

}; // end class


} } // end namespaces

#endif
