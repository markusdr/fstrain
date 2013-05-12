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
//#ifndef OBJ_FUNC_FST_R_INTERFACE_H
//#define OBJ_FUNC_FST_R_INTERFACE_H

#include "fstrain/train/obj-func-fst-factory.h"
#include "fstrain/train/obj-func-r-interface.h"

extern "C" {

  /**
   * @brief Constructs obj func object, where scoring FST is
   * read from file.
   *
   * @param func_type The func type, see
   * obj-func-fst-factory.h
   */
  void Init_FromFile(int* func_type,
                     char** fst_file, char** data_file, 
                     char** isymbols_file, char** osymbols_file, 
                     double* variance) {
    using namespace fstrain::train;
    std::string fst_file_str(*fst_file);
    std::string data_file_str(*data_file);
    std::string isymbols_file_str(*isymbols_file);
    std::string osymbols_file_str(*osymbols_file);
    ObjectiveFunctionType type = static_cast<ObjectiveFunctionType>(*func_type);
    std::cerr << "Obj. func type: " << type << std::endl;
    obj = CreateObjectiveFunctionFst_FromFile(
        type,
        fst_file_str, data_file_str, 
        isymbols_file_str, osymbols_file_str,
	*variance);
  }

  void FunctionConvergesForAnyInput(double* x, bool* result) {
    using namespace fstrain::train;
    ObjectiveFunctionFst* obj_cast = dynamic_cast<ObjectiveFunctionFst*>(obj);
    *result = ConvergesForAnyInput(obj_cast, x);
  }

  void SetFstDelta(double* d) {
    using namespace fstrain::train;
    ObjectiveFunctionFst* obj_cast = dynamic_cast<ObjectiveFunctionFst*>(obj);
    obj_cast->SetFstDelta(*d);
  }

  // TODO: using double instead of long because not sure how long is passed to R
  void SetTimelimit(double* d) {
    using namespace fstrain::train;
    ObjectiveFunctionFst* obj_cast = dynamic_cast<ObjectiveFunctionFst*>(obj);
    obj_cast->SetTimelimit((long)*d);
  }

  void Save(char** filename) {
    using namespace fstrain::train;
    std::string filename_str(*filename);
    ObjectiveFunctionFst* obj_cast = dynamic_cast<ObjectiveFunctionFst*>(obj);
    Save(*obj_cast, filename_str);
  }

  void SetNumThreads(int* n) {
    using namespace fstrain::train;
    ObjectiveFunctionFst* obj_cast = dynamic_cast<ObjectiveFunctionFst*>(obj);
    obj_cast->SetNumThreads(*n);
  }

}

// #endif
