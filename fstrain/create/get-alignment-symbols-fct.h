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
#ifndef GET_ALIGNMENT_SYMBOLS_FCT_H
#define GET_ALIGNMENT_SYMBOLS_FCT_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "fst/symbol-table.h"

namespace fstrain { namespace create {

/**
 * @brief Interface for function object: Extracts alignment symbols
 * from aligned text.
 */
struct GetAlignmentSymbolsFct {
  virtual ~GetAlignmentSymbolsFct() {}
  virtual void operator()(std::istream& aligned_data, 
                          const fst::SymbolTable& isymbols, 
                          const fst::SymbolTable& osymbols, 
                          fst::SymbolTable* syms) = 0;
};

/**
 * @brief Simply extracts alignment symbols from aligned text (without
 * adding any more symbols).
 */
struct GetAlignmentSymbolsFct_Simple : public GetAlignmentSymbolsFct {
  void operator()(std::istream& aligned_data, 
                  const fst::SymbolTable& isymbols, 
                  const fst::SymbolTable& osymbols, 
                  fst::SymbolTable* syms) {
    syms->AddSymbol("eps");
    std::string symbol;
    while(aligned_data >> symbol){
      syms->AddSymbol(symbol);
    }
  }
};

struct GetAlignmentSymbolsFct_AddIdentityChars : public GetAlignmentSymbolsFct {
  void operator()(std::istream& aligned_data, 
                  const fst::SymbolTable& isymbols, 
                  const fst::SymbolTable& osymbols, 
                  fst::SymbolTable* syms) {
    syms->AddSymbol("eps");
    std::string symbol;
    while(aligned_data >> symbol){
      syms->AddSymbol(symbol);
    }
    // add identity characters:
    fst::SymbolTableIterator sit(isymbols);
    sit.Next(); // ignore eps
    for(; !sit.Done(); sit.Next()){      
      if(osymbols.Find(sit.Symbol()) != fst::kNoLabel){
        std::stringstream ss;
        ss << sit.Symbol() << "|" << sit.Symbol();
        syms->AddSymbol(ss.str());  
      }
    }
  }
};

struct GetAlignmentSymbolsFct_AddIdentityChars_Countsyms : public GetAlignmentSymbolsFct {
  typedef std::map<std::string, std::map<int64,std::size_t> > Counts;
  Counts counts_;
  void operator()(std::istream& aligned_data, 
                  const fst::SymbolTable& isymbols, 
                  const fst::SymbolTable& osymbols, 
                  fst::SymbolTable* syms) {
    syms->AddSymbol("eps");
    std::string symbol;
    while(aligned_data >> symbol){
      int64 label = syms->AddSymbol(symbol);
      std::size_t sep_pos = symbol.find("|"); // e.g. in "a|x"
      if(sep_pos == std::string::npos) {
        FSTR_CREATE_EXCEPTION("Not an alignment char: " << symbol);
      }
      std::string input_sym = symbol.substr(0, sep_pos); // e.g. "a"
      ++counts_[input_sym][label];
    }
    // add identity characters:
    fst::SymbolTableIterator sit(isymbols);
    sit.Next(); // ignore eps
    for(; !sit.Done(); sit.Next()){      
      if(osymbols.Find(sit.Symbol()) != fst::kNoLabel){
        std::stringstream ss;
        ss << sit.Symbol() << "|" << sit.Symbol();
        syms->AddSymbol(ss.str());  
      }
    }
  }
  const Counts& GetCounts() const { return counts_; }
};

} } // end namespaces

#endif
