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
#include <iostream>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include "fstrain/train/obj-func-fst-joint.h"
#include "fstrain/train/obj-func-fst.h"
#include "fstrain/train/obj-func-fst-util.h"
#include "fst/fst.h"
#include "fst/compose.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/debug.h"
// #include "fstrain/core/fst-util.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/timer.h"
#include "fstrain/util/memory-info.h"

using namespace fst;

namespace fstrain { namespace train {

ObjectiveFunctionFstJoint::ObjectiveFunctionFstJoint(
    fst::MutableFst<fst::MDExpectationArc>* fst,      
    const util::Data* data,
    const fst::SymbolTable* isymbols,
    const fst::SymbolTable* osymbols,
    double variance)      
    : ObjectiveFunctionFst(fst),
      data_(data), isymbols_(isymbols), osymbols_(osymbols), variance_(variance)
{
  std::cerr << "# Constructing ObjectiveFunctionFstJoint" << std::endl;
  std::cerr << "# Data size: " << data_->size() << std::endl;    
  std::cerr << "# Num params: " << GetNumParameters() << std::endl;      
}

ObjectiveFunctionFstJoint::~ObjectiveFunctionFstJoint() {
  delete data_;
  delete isymbols_;
  delete osymbols_;
}

void ObjectiveFunctionFstJoint::ComputeGradientsAndFunctionValue(const double* x) {
  
  static int call_counter = -1;
  ++call_counter;
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  using core::NeglogNum;
  using core::GetOrigNum;

  util::Timer timer;
  size_t num_params = GetNumParameters();
  double* gradients = GetGradients();

  SetFunctionValue(0.0);
  for(size_t i = 0; i < num_params; ++i){
    gradients[i] = 0.0;
  }

  // regularize
  double norm = 0.0;
  for(int i = 0; i < num_params; ++i){
    norm += (x[i] * x[i]);
    gradients[i] += x[i] / variance_;
  }   
  SetFunctionValue(GetFunctionValue() + norm / (2.0 * variance_));
  
  for(size_t i = 0; i < data_->size(); ++i) {
    if(exclude_data_indices_.find(i) != exclude_data_indices_.end()){
      continue;
    }
    const std::pair<std::string, std::string>& inout = (*data_)[i];
    try {
      ProcessInputOutputPair(inout.first, inout.second);
    }
    catch(std::runtime_error) {
      std::cerr << "Warning: Ignoring example " 
                << inout.first << " / " << inout.second << std::endl;
      exclude_data_indices_.insert(i);
    }
  } // end loop over data 

  double factor = data_->size() - exclude_data_indices_.size();
  long* timelimit = GetTimelimit();
  long unlimited = (long)1e8;
  util::Timer unclamped_timer;
  double unclamped_result = GetFeatureMDExpectations<double, double*>(
      GetFst(), &gradients, num_params, 
      true, factor, 
      GetFstDelta(), call_counter == 0 ? &unlimited : timelimit);
  unclamped_timer.stop();
  SetFunctionValue(GetFunctionValue() - unclamped_result);

  std::cerr << setprecision(8) 
            << "Returning x=" << x[0] << "\tg=" << gradients[0] 
            << "\tobj=" << GetFunctionValue();
  timer.stop();
  fprintf(stderr, "\t[%2.2f ms, %2.2f MB]\n", 
          timer.get_elapsed_time_millis(), util::MemoryInfo::instance().getSizeInMB());

  if(GetFunctionValue() == NeglogNum(core::kPosInfinity)){
    SetFunctionValue(1.0);
    return;
  }
  SquashFunction();
} 

void ObjectiveFunctionFstJoint::ProcessInputOutputPair(
    const std::string& in, const std::string& out) {
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  VectorFst<MDExpectationArc> inputFst;
  VectorFst<MDExpectationArc> outputFst;
  util::ConvertStringToFst(in, *isymbols_, &inputFst);
  util::ConvertStringToFst(out, *osymbols_, &outputFst);
  ComposeFst<MDExpectationArc> unclamped(inputFst, GetFst());
  ComposeFstOptions<MDExpectationArc> copts;
  copts.gc_limit = 0;  // Cache only the last state for fastest copy.
  ComposeFst<MDExpectationArc> clamped(unclamped, outputFst, copts);            
  double* gradients = GetGradients();
  // may throw:  
  double clamped_result = GetFeatureMDExpectations<double, double*>(
      clamped, &gradients, GetNumParameters(), 
      false, 1.0,
      GetFstDelta(), GetTimelimit());       
  SetFunctionValue(GetFunctionValue() + clamped_result);
}

} } // end namespace fstrain/train
