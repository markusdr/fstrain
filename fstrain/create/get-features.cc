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
#include "get-features.h"

namespace fstrain { namespace create {

int GetNumPartsAndPrepareString(const std::string& str, std::string* result) {
  std::stringstream ss(str);
  std::string symbol;
  int max_num = 0;
  std::vector<std::string> symbols_vec;
  while (std::getline(ss, symbol, ' ')) { // symbol="S|S" or "a|x-c1" or
    // "a|--c1" etc.
    int num = 1; // "a|x"
    if (symbol.length() > 3 && symbol[3] == '-') {
      std::string la = symbol.substr(4);
      std::stringstream la_parts(la);
      std::string part;
      while (std::getline(la_parts, part, '-')) { // part="c1" or "g3"
	++num;
      }
    }
    max_num = std::max(max_num, num);
    symbols_vec.push_back(symbol);
  }
  std::string pseudo_la;
  for (int i = 0; i < max_num; ++i) {
    pseudo_la += "-??";
  }
  for (int i = 0; i < symbols_vec.size(); ++i) {
    if (symbols_vec[i].length() < 4) {
      symbols_vec[i] += pseudo_la;
    }
    *result += symbols_vec[i] +
        (i < symbols_vec.size()-1 ? " " : "");
  }
  return max_num;
}

void ChangeHyphensInSymbol(DecodeType type, std::string* symbol) {
  std::string::size_type separator_pos = symbol->find('|');
  char from = (type == ENCODE ? '-' : ' ');
  char to = (type == ENCODE ? ' ' : '-');
  if (separator_pos != std::string::npos) {
    if (separator_pos != 0
       && (*symbol)[separator_pos - 1] == from) {
      (*symbol)[separator_pos - 1] = to;
    }
    if (separator_pos + 1 < symbol->length()
      && (*symbol)[separator_pos + 1] == from) {
      (*symbol)[separator_pos + 1] = to;
    }
  }
}

} } // end namespaces
