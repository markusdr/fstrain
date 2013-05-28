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
#ifndef FSTR_CREATE_ALL_SYMBOLS_H
#define FSTR_CREATE_ALL_SYMBOLS_H

#include "fst/symbol-table.h"

namespace fstrain { namespace create {

/**
 * @brief For convenience, summarize SymbolTable and its start, end,
 * and phi symbols here.
 */
struct AllSymbols {
  const fst::SymbolTable* symbol_table;
  int64 kStartLabel, kEndLabel, kPhiLabel;  
  AllSymbols(const fst::SymbolTable* t, int64 kStartLabel_, int64 kEndLabel_, 
             int64 kPhiLabel_) 
      : symbol_table(t), 
        kStartLabel(kStartLabel_), 
        kEndLabel(kEndLabel_),
        kPhiLabel(kPhiLabel_) {}
};

} } // end namespaces

#endif
