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
#ifndef FSTRAIN_CREATE_GET_FEATURES_H
#define FSTRAIN_CREATE_GET_FEATURES_H

#include <sstream>
#include <limits>
#include <cmath>
#include <vector>
#include <iterator>
#include <algorithm>

namespace fstrain { namespace create {

/**
 * @brief Returns max num of latent annotations (e.g. 2 for
 * "a|x-c2-g3") and changes a string that has no annotations to
 * contain the max num (e.g. changes "S|S" to "S|S-??-??").
 *
 * Doing this is kind of inefficient, but prob. won't matter too much.
 */
int GetNumPartsAndPrepareString(const std::string& str, std::string* result);

enum DecodeType {ENCODE, DECODE};

/**
 * @brief Replaces hyphen around the separator with a blank (ENCODE),
 * e.g. "a|--c1" -> "a| -c1", or vice versa.
 */
void ChangeHyphensInSymbol(DecodeType type, std::string* symbol);

/**
 * @brief Returns all features for a given string, e.g. for string
 * "S|S-c1-g0 a|x-c1-g1 x|y-c1-g2": "S|S-c1-g0 a|x-c1-g1 x|y-c1-g2",
 * "c1-g0 c1-g1 c1-g2", "S|S-g0 a|x-g1 x|y-g2", "g0 g1 g2", "S|S-c1
 * a|x-c1 x|y-c1", "c1 c1 c1", "S|S a|x x|y", as well as all shorter
 * versions: "a|x x|y", "x|y", etc.
 */
template<class TSet>
void GetFeatures(const std::string& str0, TSet* featset) {
  std::string symbol;
  std::string str;
  int num_parts = GetNumPartsAndPrepareString(str0, &str);
  int bit_pattern = (int)pow(2.0, (double)num_parts) - 1; // all bits on
  while (bit_pattern > 0) {
    std::stringstream tokenizer1(str);
    std::vector<std::string> feature;
    while (std::getline(tokenizer1, symbol, ' ')) { // e.g. a|--c1
      ChangeHyphensInSymbol(ENCODE, &symbol);
      std::stringstream tokenizer2(symbol);
      std::string symbol_part;
      int bitnum = 0;
      bool first = true;
      std::string modified_symbol;
      while (std::getline(tokenizer2, symbol_part, '-')) {
	if (bit_pattern & (int)pow(2.0, (double)bitnum)) {
          ChangeHyphensInSymbol(DECODE, &symbol_part);
          modified_symbol += (!first ? "-" : "") + symbol_part;
          first = false;
	}
	++bitnum;
      }
      feature.push_back(modified_symbol);
    }
    std::vector<std::string>::iterator it = feature.begin();
    for (; it != feature.end(); ++it) {
      std::stringstream ss;
      //std::copy(it, feature.end(), std::ostream_iterator<std::string>(ss, " "));
      bool first = true;
      for (std::vector<std::string>::iterator cur = it; cur != feature.end(); ++cur) {
        ss << (first ? "" : " ") << *cur;
        first = false;
      }
      featset->insert(ss.str());
    }
    --bit_pattern;
  }
}

} } // end namespaces

#endif
