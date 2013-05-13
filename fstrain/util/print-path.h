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
#ifndef FSTRAIN_UTIL_PRINT_PATH_H
#define FSTRAIN_UTIL_PRINT_PATH_H

#include <iostream>
#include <sstream>
#include "fst/fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/debug.h"

namespace fstrain { namespace util {

void PrintLabel(int64 label, const fst::SymbolTable* syms, 
                std::ostream* out);

/**
 * @brief Helper function class for PrintPath.
 */
template<class Arc, class OutputStream>
struct PrintTransducerArcFct {
  const fst::SymbolTable* isymbols;
  const fst::SymbolTable* osymbols;
  OutputStream* out;
  std::string separator;
  PrintTransducerArcFct(const fst::SymbolTable* isymbols_, 
                        const fst::SymbolTable* osymbols_,
                        OutputStream* out_)
      : isymbols(isymbols_), osymbols(osymbols_), 
        out(out_), 
        separator("|") {}
  void operator()(const Arc& arc) {
    std::stringstream ss;
    PrintLabel(arc.ilabel, isymbols, &ss);
    ss << separator;
    PrintLabel(arc.olabel, osymbols, &ss);
    (*out) << ss.str();
  }
};

/**
 * @brief Helper function class for PrintPath.
 */
template<class Arc>
struct PrintAcceptorArcFct {
  const fst::SymbolTable* isymbols;
  std::ostream* out;
  PrintAcceptorArcFct(const fst::SymbolTable* isymbols_, 
                      std::ostream* out_ = &std::cout) 
      : isymbols(isymbols_), out(out_) {}
  void operator()(const Arc& arc) {
    PrintLabel(arc.ilabel, isymbols, out);
  }
};

/**
 * @brief Prints an FST path as string (FST must not contain more than
 * one path). Remember to do RmEpsilon if you do not want epsilons in
 * the output path.
 */
template<class Arc, class PrintArcFctT>
void PrintPath(const fst::Fst<Arc>& f, PrintArcFctT& print_arc_fct, 
               bool add_blanks = true){
  int s = f.Start();
  bool first = true;
  do {
    if(s == fst::kNoStateId){
      FSTR_UTIL_EXCEPTION("PrintPath: Not a valid path");
    }
    fst::ArcIterator< fst::Fst<Arc> > it(f, s);
    if(!first && add_blanks){
      (*print_arc_fct.out) << " ";
    }
    print_arc_fct(it.Value());
    first = false;
    s = it.Value().nextstate;
    it.Next();
    if(!it.Done()){
      FSTR_UTIL_EXCEPTION("PrintPath: Not a linear machine");
    }
  } while(f.Final(s) == Arc::Weight::Zero());
}

} } // end namespaces

#endif
