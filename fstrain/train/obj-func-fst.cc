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
#include <iomanip>

#include "fst/mutable-fst.h"

#include "fstrain/core/expectation-arc.h"

#include "fstrain/train/debug.h"
#include "fstrain/train/obj-func-fst.h"
#include "fstrain/train/set-feature-weights.h"

#include "fstrain/util/check-convergence.h"
#include "fstrain/util/get-highest-feature-index.h"
#include "fstrain/util/double-precision-weight.h"
#include "fstrain/util/memory-info.h"

using namespace fst;

namespace fstrain { namespace train {

ObjectiveFunctionFst::ObjectiveFunctionFst(MutableFst<MDExpectationArc>* fst)
    : fst_(fst), value_(0.0), fst_delta_(1e-8), timelimit_ms_(1000), num_threads_(1)
{
  std::cerr << "# Constructing ObjectiveFunctionFst" << std::endl;
  int highest_feature_index =
      (fst_ == NULL) ? -1 : fstrain::util::getHighestFeatureIndex(*fst_);
  num_params_ = highest_feature_index + 1;
  gradients_ = (double*) calloc(num_params_, sizeof(double));
  std::cerr << "# Num params: " << num_params_ << std::endl;
}

ObjectiveFunctionFst::~ObjectiveFunctionFst() {
  free(gradients_);
  delete fst_;
}

void ObjectiveFunctionFst::SetParameters(const double* x) {
  SetFeatureWeights(x, fst_);
  ComputeGradientsAndFunctionValue(x);
}

const double* ObjectiveFunctionFst::GetParameters() const {
  FSTR_TRAIN_EXCEPTION("GetParameters unimplemented");
}

void ObjectiveFunctionFst::SquashFunction() {
  double changed_val = value_ + 1;
  double divide_by = changed_val * changed_val;
  for(int i = 0; i < num_params_; ++i){
    gradients_[i] /= divide_by;
  }
  value_ /= changed_val;
}

 bool ConvergesForAnyInput(ObjectiveFunctionFst* theFunction,
                           const double* x,
                           size_t maxiter,
                           double eigenval_convergence_tol) {
  using util::LogDArc;
  SetFeatureWeights(x, theFunction->fst_);
  typedef WeightConvertMapper<MDExpectationArc, LogDArc> Map_EL;
  MapFst<MDExpectationArc, LogDArc, Map_EL> mapped(*(theFunction->fst_), Map_EL());
  util::CheckConvergenceOptions opts(maxiter, eigenval_convergence_tol);
  return util::CheckConvergence(mapped, opts);
}

void Save(const ObjectiveFunctionFst& theFunction, const std::string& filename) {
  std::cerr << "# Enter Save(), using " << util::MemoryInfo::instance().getSizeInMB()
            << " MB." << std::endl;
  theFunction.GetFst().Write(filename);
  std::cerr << "# Exit Save(), using " << util::MemoryInfo::instance().getSizeInMB()
            << " MB." << std::endl;
}

} } // end namespace fstrain/train
