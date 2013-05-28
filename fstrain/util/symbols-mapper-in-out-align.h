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
#ifndef FSTRAIN_UTIL_SYMBOLS_MAPPER_IN_OUT_TO_ALIGN_H
#define FSTRAIN_UTIL_SYMBOLS_MAPPER_IN_OUT_TO_ALIGN_H

#include "fst/symbol-table.h"
#include "fst/map.h"

namespace fstrain { namespace util {

/**
 * @brief Mapper to change transducer (e.g. a:x) to FSA (e.g a|x)
 */
template <class Arc>
class SymbolsMapper_InOutToAlign {
  typedef typename Arc::Weight Weight;
  const fst::SymbolTable& isyms_;
  const fst::SymbolTable& osyms_;
  const fst::SymbolTable& align_syms_;
  const char sep_char_;

 public:
  SymbolsMapper_InOutToAlign(
      const fst::SymbolTable& isyms,
      const fst::SymbolTable& osyms,
      const fst::SymbolTable& align_syms,
      const char sep_char = '|')
      : isyms_(isyms), osyms_(osyms), align_syms_(align_syms),
        sep_char_(sep_char) {}

  Arc operator()(const Arc &arc) const {
    using namespace fst;
    Arc new_arc = arc;
    std::string isym = arc.ilabel == 0 ? "-" : isyms_.Find(arc.ilabel);
    std::string osym = arc.olabel == 0 ? "-" : osyms_.Find(arc.olabel);
    std::stringstream ss;
    bool both_eps = arc.ilabel == 0 && arc.olabel == 0;
    if (both_eps) {
      ss << "eps";
    }
    else {
      ss << isym << sep_char_ << osym;
    }
    int64 align_label = align_syms_.Find(ss.str());
    if (align_label == kNoLabel) {
      //align_label = 1; // TEST
    }
    new_arc.ilabel = align_label;
    new_arc.olabel = align_label;
    return new_arc;
  }

  fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }

  fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}

  // signal that it is acceptor now
  uint64 Properties(uint64 props) const { return fst::ProjectProperties(props, true); }

};

} } // end namespaces

#endif
