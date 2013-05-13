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
#ifndef OBJECTIVE_FUNCTION_FST_CONDOTHER_LENMATCH_H
#define OBJECTIVE_FUNCTION_FST_CONDOTHER_LENMATCH_H

#include <set>
#include <string>
#include <vector>
#include "obj-func-fst-conditional-lenmatch.h"
#include "obj-func-fst.h"
#include "obj-func-fst-util.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/data.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace train {

/**
 * @brief Condother objective function (FST-based).
 */
class ObjectiveFunctionFstCondotherLenmatch : public ObjectiveFunctionFstConditionalLenmatch {

 public:

  /**
   * @brief Constructor; all passed objects will be deleted by
   * destructor.
   */
  ObjectiveFunctionFstCondotherLenmatch(
      fst::MutableFst<fst::MDExpectationArc>* fst,
      const fstrain::util::Data* data,
      const fst::SymbolTable* isymbols,
      const fst::SymbolTable* osymbols,
      double variance = 10.0,
      double length_variance = 10.0);

  virtual ~ObjectiveFunctionFstCondotherLenmatch();

  /**
   * @brief Sets information on the 'other' variable, which we
   * condition on.
   */
  void SetOtherVariable(const std::vector<std::string>* other_list,
                        const fst::Fst<fst::LogArc>* other_to_in_fst,
                        const fst::Fst<fst::LogArc>* other_to_out_fst,
                        const fst::SymbolTable* other_symbols);

  void SetKbest(int kbest);

 protected:

  virtual void ComputeGradientsAndFunctionValue(const double* params);

 private:

  // FIX: these vars should only be in super class
  const fstrain::util::Data* data_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  double variance_;
  std::set<int> exclude_data_indices_;

  const std::vector<std::string>* other_list_;
  const fst::Fst<fst::LogArc>* other_to_in_fst_;
  const fst::Fst<fst::LogArc>* other_to_out_fst_;
  const fst::SymbolTable* other_symbols_;
  int kbest_;

  void ProcessInputOutputPair(const std::string& in,
                              const std::string& out,
                              const std::string& other);

}; // end class

} } // end namespace fstrain/train

#endif
