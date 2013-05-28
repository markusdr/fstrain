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
#include "fstrain/util/print-path.h"
#include "fst/symbol-table.h"
#include <iostream>

namespace fstrain { namespace util {

void PrintLabel(int64 label, const fst::SymbolTable* syms,
                std::ostream* out) {
  if (syms) {
    const std::string symbol =
        label == 0
        ? "-"
        : syms->Find(label);
    (*out) << symbol;
  }
  else {
    (*out) << label;
  }
}

} } // end namespaces
