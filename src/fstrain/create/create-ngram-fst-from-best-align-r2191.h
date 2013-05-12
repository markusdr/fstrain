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
#ifndef FSTR_CREATE_CREATE_NGRAM_FST_FROM_BEST_ALIGN_R2191_H
#define FSTR_CREATE_CREATE_NGRAM_FST_FROM_BEST_ALIGN__R2191H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/prune-fct.h"
#include "fstrain/create/backoff-symbols.h"

#include <cstddef> // for size_t
#include <string>

#include <boost/tuple/tuple.hpp>

namespace fstrain { namespace create {

/**
 * @brief Creates ngram FST by aligning the data and observing ngrams.
 */
void CreateNgramFstFromBestAlign_r2191(size_t ngram_order,
                                       int max_insertions,
                                       int num_conjugations,
                                       int num_change_regions,
                                       const fst::Fst<fst::StdArc>& alignment_fst,
                                       PruneFct* prune_fct,
                                       const std::string& data_filename,
                                       const fst::SymbolTable* isymbols,
                                       const fst::SymbolTable* osymbols,
                                       features::ExtractFeaturesFct& extract_features_fct,
                                       const std::vector< boost::tuple<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr, std::size_t> >& backoff_syms_fcts,
                                       fst::SymbolTable* feature_names,
                                       fst::MutableFst<fst::MDExpectationArc>* result);

} } // end namespaces

#endif
