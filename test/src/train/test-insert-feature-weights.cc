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

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/compose.h"
#include "fst/equal.h"
#include "fst/union.h"
#include "fst/mutable-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/get-highest-feature-index.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/train/set-feature-weights.h"

int main(int argc, char** argv){
  try{
    
    using namespace fst;
    using namespace fstrain;

    if(argc != 3){
      throw std::runtime_error("Please give 2 arguments");
    }

    std::string file1(argv[1]);
    std::string file2(argv[2]);

    MutableFst<MDExpectationArc>* fst1 = VectorFst<MDExpectationArc>::Read(file1);
    MutableFst<MDExpectationArc>* fst2 = VectorFst<MDExpectationArc>::Read(file2);

    using util::getHighestFeatureIndex;
    int highest = std::max(getHighestFeatureIndex(*fst1),
			   getHighestFeatureIndex(*fst2));
    std::cerr << "# Found " << (highest + 1) << " features." << std::endl;
    
    std::vector<double> weights(highest + 1);
    for(int i = 0; i <= highest; ++i){
      weights[i] = 2.0 * (i+1);
    }

    typedef train::SetFeatureWeightsMapper<double> FSetter;
    FSetter mapper(&weights[0]);
    typedef MapFst<MDExpectationArc, MDExpectationArc, FSetter> FSetFst;

    ComposeFst<MDExpectationArc> composed(UnionFst<MDExpectationArc>(*fst1, *fst2), *fst2);

    //    std::cout << "Composed:" << std::endl;
    //    util::printTransducer(&composed, NULL, NULL, std::cout);

    std::cout << "Compose, then insert weights:" << std::endl;
    FSetFst result1(composed, mapper);
    util::printTransducer(&result1, NULL, NULL, std::cout);

    FSetFst inserted1(*fst1, mapper);
    FSetFst inserted2(*fst2, mapper);

    //    std::cout << "Inserted 1:" << std::endl;
    //    util::printTransducer(&inserted1, NULL, NULL, std::cout);
    //    std::cout << "Inserted 2:" << std::endl;
    //    util::printTransducer(&inserted2, NULL, NULL, std::cout);

    std::cout << "Insert weights, then compose:" << std::endl;
    ComposeFst<MDExpectationArc> result2(UnionFst<MDExpectationArc>(inserted1, inserted2), inserted2);
    util::printTransducer(&result2, NULL, NULL, std::cout);

    if(Equal(result1, result2)){
      std::cout << "OK" << std::endl;
    }
    else {
      std::cout << "FAIL" << std::endl;
      throw std::runtime_error("FAIL");
    }
    
    delete fst1;
    delete fst2;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
