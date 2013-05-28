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
#include <vector>
#include "fst/fst.h"
#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/train/lenmatch.h"
#include "fstrain/train/length-feat-mapper.h"
#include "fstrain/util/get-highest-feature-index.h"

int main(int argc, char** argv){
  try{

    using namespace fst;
    using namespace fstrain;

    if(argc != 2){
      throw std::runtime_error("Please give FST as input");
    }

    std::string filename(argv[1]);
    MutableFst<MDExpectationArc>* fst = VectorFst<MDExpectationArc>::Read(filename);

    int highest = util::getHighestFeatureIndex(*fst);
    int num_feats = highest + 1;
    std::cerr << "# Found " << num_feats << " features." << std::endl;

    core::MDExpectations expected_lengths;
    core::MDExpectations empirical_lengths;
    empirical_lengths.insert(0, -log(4.0)); // set desired output length

    std::vector<double> gradients(num_feats);
    long timelimit_ms = 1000;

    train::AddLengthRegularizationOpts opts(empirical_lengths);
    opts.gradients = &gradients[0];
    opts.num_params = num_feats;
    opts.shortestdistance_timelimit = &timelimit_ms;
    opts.expected_lengths = &expected_lengths;
    opts.fst_delta = 1e-8;

    typedef train::LengthFeatMapper_Y<MDExpectationArc> LengthFeatMapper;
    double expected_length = train::AddLengthRegularization<LengthFeatMapper>(*fst, opts);

    std::cout << "Expected length: " << expected_length << std::endl;
    std::cout << "Gradients:" << std::endl;
    for(int i = 0; i < num_feats; ++i){
      std::cout << i << ": " << gradients[i] << std::endl;
    }

    delete fst;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
