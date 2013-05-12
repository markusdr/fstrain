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
#ifndef OBJECTIVE_FUNCTION_FST_CONDITIONAL_LENMATCH_H
#define OBJECTIVE_FUNCTION_FST_CONDITIONAL_LENMATCH_H

#include <set>
#include <string>
#include "obj-func-fst.h"
#include "obj-func-fst-util.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/data.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace train {

/**
 * @brief Conditional objective function (FST-based).
 */
class ObjectiveFunctionFstConditionalLenmatch : public ObjectiveFunctionFst {

 public:

  /**
   * @brief Constructor; all passed objects will be deleted by
   * destructor.
   */
  ObjectiveFunctionFstConditionalLenmatch(fst::MutableFst<fst::MDExpectationArc>* fst,      
                                          const fstrain::util::Data* data,
                                          const fst::SymbolTable* isymbols,
                                          const fst::SymbolTable* osymbols,
					  double variance = 10.0,
                                          double length_variance = 10.0); 

  virtual ~ObjectiveFunctionFstConditionalLenmatch();

  /**
   * @brief Sets variance of length regularizer
   */
  void SetLengthVariance(double variance);

  double GetLengthVariance();

  /**
   * @brief Should we match x and y length (instead of just y?)
   */
  void SetMatchXY(bool val);

  bool GetMatchXY();

 protected:
  
  virtual void ComputeGradientsAndFunctionValue(const double* params);

 private:

  const fstrain::util::Data* data_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  double variance_;
  double length_variance_;
  std::set<int> exclude_data_indices_;
  bool match_xy_;

  void ProcessInputOutputPair(const std::string& in, const std::string& out,
                              int iteration);

}; // end class

} } // end namespace fstrain/train

#endif
