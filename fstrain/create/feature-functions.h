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
#ifndef FSTRAIN_CREATE_FEATURE_FUNCTIONS_H
#define FSTRAIN_CREATE_FEATURE_FUNCTIONS_H

#include <iostream>
#include <string>
#include <sstream>

namespace fstrain { namespace create {

/**
 * @brief Adds a feature that fires whenever a character is emitted
 * (separately for input and output).
 */
template<class SetT>
void AddLengthPenalties(const std::string& symbol,
                        const char sep_char,
                        const char eps_char,
                        SetT* featset) {
  std::string in, out;
  std::stringstream arcsym_ss(symbol);
  std::getline(arcsym_ss, in, sep_char);
  std::getline(arcsym_ss, out, sep_char);
  if (in[0] != eps_char) {
    featset->insert("*LENGTH_IN*");
  }
  if (out[0] != eps_char) {
    featset->insert("*LENGTH_OUT*");
  }
}

} } // end namespaces

#endif
