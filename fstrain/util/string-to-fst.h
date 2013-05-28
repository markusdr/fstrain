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
#ifndef FSTRAIN_UTIL_STRINGTOFST_H
#define FSTRAIN_UTIL_STRINGTOFST_H

#include <string>
#include <sstream>
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/debug.h"

namespace fstrain { namespace util {

  /**
   * @brief Converts a string into a flat-line finite-state machine.
   * @param The string to convert; it is read token by token.
   */
  template<class Arc>
    void ConvertStringToFst(const std::string& str, const fst::SymbolTable& syms,
			    typename Arc::Weight final_weight,
			    fst::MutableFst<Arc>* ofst, bool delete_unknown = false) {
    using namespace fst;
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Weight Weight;
    StateId prevState = ofst->AddState();
    ofst->SetStart(prevState);
    StateId next_state = kNoStateId;
    std::stringstream ss(str);
    std::string token;
    while(ss >> token){
      next_state = ofst->AddState();
      int64 label = syms.Find(token);
      if(label == -1) {
        if(delete_unknown) {
          std::cerr << "Warning: Removed unknown token '" << token
                    << "' from " << str << "." << std::endl;
          label = 0; // replace with eps
        }
        else {
          FSTR_UTIL_EXCEPTION("Could not find id for token '" << token
                              << "' when trying to convert '" <<str << "'");
        }
      }
      ofst->AddArc(prevState, Arc(label, label, Weight::One(), next_state));
      prevState = next_state;
    }
    if(next_state != kNoStateId){
      ofst->SetFinal(next_state, final_weight);
    }
  }

  template<class Arc>
    void ConvertStringToFst(const std::string& str, const fst::SymbolTable& syms,
			    fst::MutableFst<Arc>* ofst, bool delete_unknown = false) {
    typename Arc::Weight final_weight = Arc::Weight::One();
    ConvertStringToFst(str, syms, final_weight, ofst, delete_unknown);
  }

} } // end namespaces

#endif
