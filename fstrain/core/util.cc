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
#include "fstrain/core/util.h"

namespace fstrain { namespace core {

// TODO: use StrToInt64 (see text-io.h)
unsigned long stringToUnsignedLong(const std::string& s) {
  unsigned long i;
  char *check;
  i = strtol(s.c_str(), &check, 10);
  if (check < s.c_str() + s.size()) {
    std::stringstream ss;
    ss << "Format error: Not an int value (" << s << ")";
    throw std::runtime_error(ss.str());
  }
  return i;
}

double stringToDouble(const std::string& s) {
  double d;
  if (s == "Infinity") {
    d = kPosInfinity;
  } else if (s == "-Infinity") {
    d = kNegInfinity;
  }
  else{
    char *check;
    d = strtod(s.c_str(), &check);
    if (check < s.c_str() + s.size()) {
      std::stringstream ss;
      ss << "Format error: Not a double value (" << s << ")";
      throw std::runtime_error(ss.str());
    }
  }
  return d;
}

void tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters,
              bool removeLeadingBlanks) {
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos)
  {
    if (removeLeadingBlanks) {
      while (str[lastPos] == ' ') {
        ++lastPos;
      }
    }
    // Found a token, add it to the std::vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

//  double log_add(double lx,double ly) {
//    if (lx == kNegInfinity)
//      return ly;
//    if (ly == kNegInfinity)
//      return lx;
//    double d = lx - ly;
//    if (d >= 0) {
//      if (d > 745) {
//      return lx;
//      } else {
//      return lx + log1p(exp(-d));
//      }
//    } else {
//      if (d < -745) {
//      return ly;
//      } else {
//      return ly + log1p(exp(d));
//      }
//    }
//  }

} } // end namespace fstrain/core
