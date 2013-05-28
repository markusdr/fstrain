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
#ifndef FSTRAIN_CREATE_CREATE_CREATE_NGRAM_FST_H
#define FSTRAIN_CREATE_CREATE_CREATE_NGRAM_FST_H

#include "fst/fst.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/construct-lattice-fct.h"
#include "fstrain/create/create-ngram-trie.h"
#include "fstrain/create/create-scoring-fst-from-trie.h"
#include "fstrain/create/ngram-fsa-insert-features.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/create-alignment-symbols.h"
#include "fstrain/util/options.h"
#include "fstrain/util/print-fst.h"
#include <cstddef> // for size_t
#include <string>

namespace fstrain { namespace create {

/**
 * @brief Creates ngram FST over two-dim characters with features,
 * max-ins, conjugations and change regions.
 */
void CreateNgramFst(size_t ngram_order,
                    int max_insertions,
                    const std::string& isymbols_filename,
                    const std::string& osymbols_filename,
                    features::ExtractFeaturesFct& extract_features_fct,
                    fst::SymbolTable* feature_names,
                    fst::MutableFst<fst::MDExpectationArc>* result) {
  using namespace fst;
  FSTR_CREATE_DBG_MSG(1, "MAX_INSERTIONS: " << max_insertions << std::endl);
  const SymbolTable* isymbols = SymbolTable::ReadText(isymbols_filename);
  const SymbolTable* osymbols = SymbolTable::ReadText(osymbols_filename);

  int num_conjugations = 0;
  int num_change_regions = 0;
  bool symmetric = false;
  ConstructLatticeFct_UpDown construct_lattice_fct(max_insertions,
                                                   num_conjugations,
                                                   num_change_regions,
                                                   symmetric);

  Fst<StdArc>* proj_up = construct_lattice_fct.GetProjUpFst();
  Fst<StdArc>* proj_down = construct_lattice_fct.GetProjDownFst();
  Fst<StdArc>* wellformed_fst = construct_lattice_fct.GetWellformedFst();

  // options["force-convergence"] = true; // insert length features
  util::options["eigenvalue-maxiter-checkvalue"] = true;

  SymbolTable* align_syms = new SymbolTable("align-syms");
  CreateAlignmentSymbols(*isymbols, *osymbols, "S", "E", align_syms);
  construct_lattice_fct.SetAlignmentSymbols(align_syms);
  delete align_syms;
  align_syms = (SymbolTable*)proj_up->OutputSymbols()->Copy();
  int64 kStartLabel = align_syms->Find("S|S");
  assert(kStartLabel != -1);
  int64 kEndLabel = align_syms->Find("E|E");
  assert(kEndLabel != -1);
  CreateNgramTrieOptions<MDExpectationArc> opts(ngram_order,
                                              *align_syms,
                                              kStartLabel, kEndLabel);
  // MutableFst<MDExpectationArc>* result = new VectorFst<MDExpectationArc>();
  CreateNgramTrie(result, opts);

  // TrieInsertFeatures(*align_syms, feature_names, extract_features_fct, "", result);
  fstrain::create::NgramFsaInsertFeaturesFct insert_feats_fct(*align_syms, feature_names,
                                                              extract_features_fct, "");
  AllSymbols all_align_syms(align_syms, kStartLabel, kEndLabel, -3);
  CreateScoringFstFromTrie(result, all_align_syms, *all_align_syms.symbol_table, *isymbols, *osymbols, ngram_order,
                           proj_up,
                           proj_down,
                           wellformed_fst,
                           insert_feats_fct);

  FSTR_CREATE_DBG_EXEC(100,
                       util::printTransducer(result,
                                             result->InputSymbols(),
                                             result->OutputSymbols(),
                                             std::cout););
  result->SetInputSymbols(NULL);
  result->SetOutputSymbols(NULL);

}

} } // end namespaces

#endif
