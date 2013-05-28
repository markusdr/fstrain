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
#ifndef FSTRAIN_TRAIN_OBJ_FUNC_R_INTERFACE_H
#define FSTRAIN_TRAIN_OBJ_FUNC_R_INTERFACE_H

#include <iostream>
#include "fstrain/train/obj-func.h"

extern "C" {

  fstrain::train::ObjectiveFunction* obj;

  // CreateObjectiveFunction() should be provided by an interface for
  // a specific ObjectiveFunction

  void Cleanup() {
    delete obj;
  }

  void GetNumParameters(int* result) {
    int result0 = obj->GetNumParameters();
    std::cerr << "GetNumParameters result: " << result0 << std::endl;
    *result = result0;
  }

  void GetFunctionValue(double* x, double* result) {
    obj->SetParameters(x);
    *result = obj->GetFunctionValue();
  }

  void GetGradients(double* x, double* result) {
    const double* gr = obj->GetGradients();
    size_t n = obj->GetNumParameters();
    for(size_t i = 0; i < n; ++i){
      result[i] = gr[i];
    }
  }

}

#endif
