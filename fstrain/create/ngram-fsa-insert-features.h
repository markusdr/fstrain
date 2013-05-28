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
#ifndef NGRAM_FSA_INSERT_FEATURES_H
#define NGRAM_FSA_INSERT_FEATURES_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/features/extract-features.h"

#include <string>

namespace fstrain { namespace create { 

void NgramFsaInsertFeatures(const fst::SymbolTable& symbols, 
                            fst::SymbolTable* feature_ids,
                            fstrain::create::features::ExtractFeaturesFct& extract_feats_fct, 
                            const std::string prefix,
                            fst::MutableFst<fst::MDExpectationArc>* fst);

/**
 * @brief Simple wrapper around NgramFsaInsertFeatures function
 */
struct NgramFsaInsertFeaturesFct {

  const fst::SymbolTable& syms;
  fst::SymbolTable* feature_ids;
  features::ExtractFeaturesFct& extract_features_fct;
  const std::string prefix;

  NgramFsaInsertFeaturesFct(const fst::SymbolTable& syms_,
                            fst::SymbolTable* feature_ids_,
                            features::ExtractFeaturesFct& extract_features_fct_,
                            const std::string prefix_) 
      : syms(syms_),
        feature_ids(feature_ids_),
        extract_features_fct(extract_features_fct_),
        prefix(prefix_) {}

  template<class Arc>
  void operator()(fst::MutableFst<Arc>* ngram_fsa) {
    NgramFsaInsertFeatures(syms, feature_ids, 
                           extract_features_fct, prefix, 
                           ngram_fsa);
  }

};

} } // end namespaces

#endif
