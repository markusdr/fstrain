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
#ifndef FSTRAIN_TRAIN_OBJ_FUNC_FST_H
#define FSTRAIN_TRAIN_OBJ_FUNC_FST_H

#include <cstddef>
#include "obj-func.h"
#include "fst/mutable-fst.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace train {

/**
 * @brief Base class for obj funcs that are computed by
 * FSTs.
 *
 * @todo Rename this class because ObjectiveFunctionFst sounds like a
 * specific FST type, whereas "ObjectiveFunction (based on Fsts)" is
 * meant.
 */
class ObjectiveFunctionFst : public ObjectiveFunction {
  
 public:

  /**
   * @param fst The model FST (will be deleted by destructor)
   */
  explicit
  ObjectiveFunctionFst(fst::MutableFst<fst::MDExpectationArc>* fst);
  
  virtual ~ObjectiveFunctionFst();
    
  virtual void SetParameters(const double*);

  virtual const double* GetParameters() const;

  virtual size_t GetNumParameters() const {
    return num_params_;
  }

  virtual double GetFunctionValue() const {
    return value_;
  }

  virtual const double* GetGradients() const {
    return gradients_;
  }
  
  virtual const fst::Fst<fst::MDExpectationArc>& GetFst() const {
    return *fst_;
  }

  virtual void SetTimelimit(long l) {
    if(l != timelimit_ms_){
      std::cerr << "Setting new timelimit " << l << std::endl;
      timelimit_ms_ = l;
    }
  }

  virtual long* GetTimelimit() {
    return &timelimit_ms_;
  } 

  virtual void SetFstDelta(double d) {
    fst_delta_ = d;
  }

  virtual double GetFstDelta() const {
    return fst_delta_;
  }

  virtual void Accept(ObjectiveFunctionVisitor* visitor) {
    visitor->Visit(this);
  }

  void SetNumThreads(int n) {
    num_threads_ = n;
  }

 protected:

  double* GetGradients() {
    return gradients_;
  }

  void SetFunctionValue(double v) {
    value_ = v;
  }

  virtual void ComputeGradientsAndFunctionValue(const double* params) = 0;

  /* 
   * @brief Modifies func (value and gradients) so highest f(x) =
   * 1.0
   */
  virtual void SquashFunction();

  /**
   * @brief Returns true if func converges for any input.
   * 
   * Note: If false is returned it can still converge for a given data
   * set if func is conditional (is_joint_prob_ = false).
   *
   * Note: This will set the FST feature weights, but will not update the
   * func value and gradients.
   */  
  friend bool ConvergesForAnyInput(ObjectiveFunctionFst* theFunction, 
                                   const double* x, 
                                   size_t maxiter,
                                   double eigenval_convergence_tol);

  int GetNumThreads() const { return num_threads_; }

 private:

  fst::MutableFst<fst::MDExpectationArc>* fst_;
  double value_;
  int num_params_;
  double* gradients_;
  double fst_delta_;
  long timelimit_ms_;
  int num_threads_;

};

void Save(const ObjectiveFunctionFst& theFunction, const std::string& filename);

bool ConvergesForAnyInput(ObjectiveFunctionFst* theFunction, 
                          const double* x, 
                          size_t maxiter = 20000,
                          double eigenval_convergence_tol = 1e-12);

} } // end namespace fstrain/train

#endif
