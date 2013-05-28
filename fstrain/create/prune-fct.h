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
#ifndef FSTRAIN_CREATE_PRUNE_FCT_H
#define FSTRAIN_CREATE_PRUNE_FCT_H

#include <string>
#include "fst/mutable-fst.h"
#include "fst/fst.h"
#include "fst/prune.h"

namespace fstrain { namespace create {

struct PruneFct {
  virtual ~PruneFct() {}
  virtual void operator()(const std::string& in_str, const std::string& out_str,
                          fst::MutableFst<fst::StdArc>* f) = 0;
};

struct DefaultPruneFct : public PruneFct {
  fst::StdArc::Weight weight_threshold_;
  double state_factor_;
  virtual ~DefaultPruneFct() {}
  DefaultPruneFct(fst::StdArc::Weight weight_threshold,
                  double state_factor)
      : weight_threshold_(weight_threshold), state_factor_(state_factor) {}
  virtual void operator()(const std::string& in_str, const std::string& out_str,
                          fst::MutableFst<fst::StdArc>* f) {
    fst::StdArc::StateId state_threshold =
        (fst::StdArc::StateId)(std::max(in_str.length(), out_str.length()) * state_factor_);
    std::cerr << f->NumStates() << " => ";
    fst::Prune(f, weight_threshold_, state_threshold);
    std::cerr << f->NumStates() << std::endl;
  }
};

}} // end namespaces

#endif
