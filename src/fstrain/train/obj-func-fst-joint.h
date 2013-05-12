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
#ifndef OBJ_FUNC_FST_JOINT_H
#define OBJ_FUNC_FST_JOINT_H

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
 * @brief Joint obj func (FST-based).
 */
class ObjectiveFunctionFstJoint : public ObjectiveFunctionFst {

 public:

  /**
   * @brief Constructor; all passed objects will be deleted by
   * destructor.
   */
  ObjectiveFunctionFstJoint(fst::MutableFst<fst::MDExpectationArc>* fst,      
                            const fstrain::util::Data* data,
                            const fst::SymbolTable* isymbols,
                            const fst::SymbolTable* osymbols,
                            double variance = 10.0); 

  virtual ~ObjectiveFunctionFstJoint();

 protected:
  
  virtual void ComputeGradientsAndFunctionValue(const double* params);

 private:

  const fstrain::util::Data* data_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  double variance_;
  std::set<int> exclude_data_indices_;

  void ProcessInputOutputPair(const std::string& in, const std::string& out);

}; // end class

} } // end namespace fstrain/train

#endif
