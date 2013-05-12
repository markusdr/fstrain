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
#ifndef FST_EXPECTATION_ARC_H
#define FST_EXPECTATION_ARC_H

#include "fst/arc.h"
#include "fstrain/core/expectation-weight.h"

namespace fst {
  typedef ArcTpl<MDExpectationWeight> MDExpectationArc;     
}

#endif
