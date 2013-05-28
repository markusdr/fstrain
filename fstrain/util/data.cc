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
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include "fstrain/util/trim.h"
#include "fstrain/util/data.h"

namespace fstrain { namespace util {

void Data::init_from_stream(std::istream& in) {
  while (!in.eof()) {
    std::string input;
    std::string output;
    std::getline(in, input);
    std::getline(in, output);
    if (input.length() && output.length()) {
      input = trim(input);
      output = trim(output);
      data_.push_back(std::make_pair(input, output));
    }
  }
}

void Data::init(const char* filename) {
  std::ifstream strm(filename);
  if (!strm.is_open()) {
    throw std::runtime_error("Could not open data file '" + std::string(filename) + "'");
  }
  init_from_stream(strm);
}

void Data::push_back(const std::string& in, const std::string& out) {
  data_.push_back(std::make_pair(in, out));
}


} } // end namespace fstrain::util
