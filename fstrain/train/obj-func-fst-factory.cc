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
#include "fstrain/train/obj-func-fst-factory.h"
#include "fstrain/train/obj-func-fst-joint.h"
#include "fstrain/train/obj-func-fst-conditional.h"
#include "fstrain/train/obj-func-fst-conditional-lenmatch.h"
#include "fstrain/train/obj-func-fst-condother-lenmatch.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/misc.h"
#include "fstrain/train/debug.h"

#include "fst/map.h"
#include "fst/compose.h"
#include "fstrain/util/options.h"

namespace fstrain { namespace train {

using namespace fst;

ObjectiveFunctionFst* CreateObjectiveFunctionFst(
    ObjectiveFunctionType type,
    MutableFst<MDExpectationArc>* fst, 
    const std::string& data_filename,
    SymbolTable* isymbols,
    SymbolTable* osymbols,
    double variance) 
{
  ObjectiveFunctionFst* result = NULL;
  if(type == OBJ_JOINT){
    result = new ObjectiveFunctionFstJoint(fst,
					   new fstrain::util::Data(data_filename),
					   isymbols, osymbols,
					   variance);
  }
  else if(type == OBJ_CONDITIONAL){
    result = new ObjectiveFunctionFstConditional(fst,
						 new fstrain::util::Data(data_filename),
						 isymbols, osymbols,
						 variance);
  }
  else if(type == OBJ_CONDITIONAL_LENMATCH){
    result = new ObjectiveFunctionFstConditionalLenmatch(fst,
							 new fstrain::util::Data(data_filename),
							 isymbols, osymbols,
							 variance);
  }
  else if(type == OBJ_CONDOTHER_LENMATCH){
    result = new ObjectiveFunctionFstCondotherLenmatch(fst,
						       new fstrain::util::Data(data_filename),
						       isymbols, osymbols,
						       variance);
  }
  else {
    FSTR_TRAIN_EXCEPTION("Unknown obj func type");
  }
  return result;
}

ObjectiveFunctionFst* CreateObjectiveFunctionFst(
    ObjectiveFunctionType type,
    MutableFst<MDExpectationArc>* fst, 
    const std::string& data_filename,
    const std::string& isymbols,
    const std::string& osymbols,
    double variance) 
{
  fstrain::util::ThrowExceptionIfFileNotFound(isymbols);
  fstrain::util::ThrowExceptionIfFileNotFound(osymbols);
  return CreateObjectiveFunctionFst(type, fst, data_filename, 
				    SymbolTable::ReadText(isymbols),
				    SymbolTable::ReadText(osymbols),
				    variance);
}

ObjectiveFunctionFst* CreateObjectiveFunctionFst_FromFile(
    ObjectiveFunctionType type,
    const std::string& fst_filename,
    const std::string& data_filename,
    const std::string& isymbols_filename,
    const std::string& osymbols_filename,
    double variance) 
{
  return CreateObjectiveFunctionFst(
      type,
      VectorFst<MDExpectationArc>::Read(fst_filename),
      data_filename,
      SymbolTable::ReadText(isymbols_filename),
      SymbolTable::ReadText(osymbols_filename),
      variance);
}

} } // end namespaces 
