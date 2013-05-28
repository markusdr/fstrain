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
#ifndef FSTRAIN_TRAIN_OBJ_FUNC_FST_CONDITIONAL_H
#define FSTRAIN_TRAIN_OBJ_FUNC_FST_CONDITIONAL_H

#include <set>
#include <string>
#include "obj-func-fst.h"
#include "obj-func-fst-util.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/data.h"
#include "fstrain/util/compose-fcts.h"
#include "fstrain/core/expectation-arc.h"
#include <boost/thread/mutex.hpp>

namespace fstrain { namespace train {

/**
 * @brief Conditional obj func (FST-based).
 */
class ObjectiveFunctionFstConditional : public ObjectiveFunctionFst {

 public:

  /**
   * @brief Constructor; all passed objects will be deleted by
   * destructor.
   *
   * @param compose_input_fct A function that composes an input with
   * the scoring FSM (this may involve simple composition or some
   * other connection step, e.g. using proj_up and proj_down.)
   * @param compose_output_fct A function that composes scoring FSM with the output.
   */
  ObjectiveFunctionFstConditional(fst::MutableFst<fst::MDExpectationArc>* fst,
                                  const fstrain::util::Data* data,
                                  const fst::SymbolTable* isymbols,
                                  const fst::SymbolTable* osymbols,
                                  double variance = 10.0,
                                  util::ComposeFct<fst::MDExpectationArc>* compose_input_fct = new util::DefaultComposeFct<fst::MDExpectationArc>(),
                                  util::ComposeFct<fst::MDExpectationArc>* compose_output_fct = new util::DefaultComposeFct<fst::MDExpectationArc>())
      : ObjectiveFunctionFst(fst),
        data_(data), isymbols_(isymbols), osymbols_(osymbols), variance_(variance),
        compose_input_fct_(compose_input_fct), compose_output_fct_(compose_output_fct)
  {
    std::cerr << "# Constructing ObjectiveFunctionFstConditional" << std::endl;
    std::cerr << "# Data size: " << data_->size() << std::endl;
    std::cerr << "# Num params: " << GetNumParameters() << std::endl;
  }

  virtual ~ObjectiveFunctionFstConditional();

 protected:

  virtual void ComputeGradientsAndFunctionValue(const double* params);

 private:

  const fstrain::util::Data* data_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  double variance_;
  std::set<int> exclude_data_indices_;
  util::ComposeFct<fst::MDExpectationArc>* compose_input_fct_;
  util::ComposeFct<fst::MDExpectationArc>* compose_output_fct_;
  boost::mutex mutex_gradient_access_;
  boost::mutex mutex_functionval_access_;

  void ProcessInputOutputPair(const std::string& in, const std::string& out,
                              int iteration);

  friend struct ProcessInputOutputPair_Fct;

}; // end class

} } // end namespace fstrain/train

#endif
