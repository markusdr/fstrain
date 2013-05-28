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

#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/util/shortest-distance-m.h"

int main(int argc, char** argv) {
  try {

    fst::MutableFst<fst::LogArc>* fst =
        fst::VectorFst<fst::LogArc>::Read(std::string(argv[1]));

    std::vector<fst::LogArc::Weight> alphas;
    fstrain::util::ShortestDistanceM(*fst, &alphas, true, 1e-4);

    std::cout << std::setprecision(12);
    for(int i = 0; i < alphas.size(); ++i) {
      std::cout << i << " " << alphas[i] << std::endl;
    }

    delete fst;

  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
