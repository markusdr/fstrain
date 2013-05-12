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
