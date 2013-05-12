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
// Source for expectation arc shared object.

#include "fst/mains.h"
//#include "fst/script/fstscript.h"
#include "fst/vector-fst.h"
#include "fst/const-fst.h"
#include "fstrain/core/expectation-arc.h"

#ifdef INLINE_EVERYTHING
#include "fstrain/core/util.cc"
#include "fstrain/core/expectations.cc"
#endif

using fst::VectorFst;
using fst::ConstFst;
using fst::MDExpectationArc;

extern "C" {

void expectation_arc_init() {
  REGISTER_FST(VectorFst, MDExpectationArc);
  REGISTER_FST(ConstFst, MDExpectationArc);
  fst::RegisterFstMains<MDExpectationArc>();
  // for OpenFst 1.2:
  // fst::script::AllFstOperationsRegisterer<MDExpectationArc>();
}

}
