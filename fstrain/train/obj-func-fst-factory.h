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
#ifndef FSTRAIN_TRAIN_OBJ_FUNC_FST_FACTORY_H
#define FSTRAIN_TRAIN_OBJ_FUNC_FST_FACTORY_H

#include <cstddef>
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "obj-func-fst.h"

namespace fstrain { namespace train {

enum ObjectiveFunctionType { OBJ_JOINT = 0, 
			     OBJ_CONDITIONAL = 1, 
			     OBJ_CONDITIONAL_LENMATCH = 2,
			     OBJ_CONDOTHER_LENMATCH = 3};

/**
 * @brief Factory function that creates an objective function using a
 * given FST that contains all features and transitions needed to
 * model the domain.
 */
ObjectiveFunctionFst* CreateObjectiveFunctionFst(
    ObjectiveFunctionType type,
    fst::MutableFst<fst::MDExpectationArc>* fst, 
    const std::string& dataFilename,
    const std::string& isymbolsFilename,
    const std::string& osymbolsFilename,
    double variance);

/**
 * @brief Factory function that creates an objective function using an
 * FST that is loaded from disk.
 */
ObjectiveFunctionFst* CreateObjectiveFunctionFst_FromFile(
    ObjectiveFunctionType type,
    const std::string& fstFilename,
    const std::string& dataFilename,
    const std::string& isymbolsFilename,
    const std::string& osymbolsFilename,
    double variance);

} } // end namespaces

#endif
