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
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <fst/mutable-fst.h>
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/expectations.h"
#include "fstrain/util/print-fst.h"

using namespace fst;

int main(int argc, char** argv) {
  try{
    using fstrain::core::NeglogNum;
    typedef MDExpectationArc::StateId StateId;
    typedef MDExpectationArc::Weight Weight;
    MutableFst<MDExpectationArc>* fst = new VectorFst<MDExpectationArc>();

    StateId start_state = fst->AddState();
    fst->SetStart(start_state);

    StateId final_state = fst->AddState();
    fst->SetFinal(final_state, MDExpectationWeight::One());

    double arc_neglog_prob = -log(0.5);
    Weight w(arc_neglog_prob);
    fstrain::core::MDExpectations& e = w.GetMDExpectations();
    e.insert(0, NeglogNum(arc_neglog_prob + -log(1.0))); // fire once
    e.insert(1, NeglogNum(arc_neglog_prob + -log(2.0))); // fire twice
    fst->AddArc(start_state,
                MDExpectationArc(10, 100, w, final_state));
    fstrain::util::printTransducer(fst, NULL, NULL, std::cerr);
    fst->Write(std::cout, FstWriteOptions()); // writes binary
  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
