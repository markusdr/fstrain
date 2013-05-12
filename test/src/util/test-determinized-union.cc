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
#include <stdexcept>
#include <queue>

#include <boost/foreach.hpp>

#include "fst/fst.h"
#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"

#include "fstrain/util/determinized-union.h"
#include "fstrain/util/print-fst.h"

int main(int argc, char** argv){
  try{

    if(argc != 3) {
      std::cerr << "Please give 2 FSTs as arguments" << std::endl;
      exit(1);
    }

    const std::string fst1_name = argv[1];
    const std::string fst2_name = argv[2];

    using namespace fst;

    MutableFst<LogArc>* fst1 = VectorFst<LogArc>::Read(fst1_name);
    if(fst1 == NULL) {
      std::cerr << "Could not load " << fst1_name << std::endl;
      exit(1);
    }
    MutableFst<LogArc>* fst2 = VectorFst<LogArc>::Read(fst2_name);    
    if(fst2 == NULL) {
      std::cerr << "Could not load " << fst2_name << std::endl;
      exit(1);
    }

    fstrain::util::DeterminizedUnion<LogArc>(fst1, *fst2);
    fstrain::util::printTransducer(fst1, NULL, NULL, std::cout);

    delete fst1;
    delete fst2;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
