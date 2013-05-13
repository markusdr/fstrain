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
#ifndef UTIL_DOUBLE_PRECISION_WEIGHT_H
#define UTIL_DOUBLE_PRECISION_WEIGHT_H

#include "fst/float-weight.h"

namespace fstrain { namespace util {

// Double precision 
typedef fst::LogWeightTpl<double> LogDWeight;
typedef fst::ArcTpl<LogDWeight> LogDArc;

} } // end namespace fstrain::util

#endif
