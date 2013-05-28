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

#include "fst/fst.h"
#include "fst/map.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/arcsort.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/count-ngrams-in-data.h"
#include "fstrain/create/create-ngram-trie.h"
#include "fstrain/create/create-scoring-fst-from-trie.h"
// #include "fstrain/create/trie-insert-features.h"
#include "fstrain/create/ngram-fsa-insert-features.h"
#include "fstrain/create/add-unobserved-unigrams.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/prune-fct.h"
#include "fstrain/create/debug.h"
#include "fstrain/create/create-backoff-model.h"
#include "fstrain/create/backoff-symbols.h"

#include "fstrain/util/options.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/timer.h"
#include "fstrain/util/memory-info.h"
#include "fstrain/util/intersect-vec.h"

#include <cstddef> // for size_t
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

namespace fstrain { namespace create {

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
                                       const std::vector< boost::tuple<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr, std::size_t> >& backoff,
                                       fst::SymbolTable* feature_names,
                                       fst::MutableFst<fst::MDExpectationArc>* result) {
  using namespace fst;
  util::Data data(data_filename);

  const bool symmetric = false;
  const int max_insertions_nolimit = -1;
  ConstructLatticeFct_UpDown construct_lattice_fct(max_insertions_nolimit,
                                                   num_conjugations,
                                                   num_change_regions,
                                                   symmetric);

  Fst<StdArc>* proj_up = construct_lattice_fct.GetProjUpFst();
  Fst<StdArc>* proj_down = construct_lattice_fct.GetProjDownFst();
  Fst<StdArc>* wellformed_fst = construct_lattice_fct.GetWellformedFst();

  util::Timer timer;
  SymbolTable* align_syms = NULL;
  GetAlignmentSymbolsFct_AddIdentityChars get_align_syms_fct;
  CountNgramsInData(data, *isymbols, *osymbols,
                    alignment_fst, prune_fct,
                    &get_align_syms_fct, &construct_lattice_fct,
                    ngram_order,
                    align_syms,
                    result);

  FSTR_CREATE_DBG_EXEC(10,
                       std::cerr << "Trie 1:" << std::endl;
                       util::printTransducer(result, align_syms, align_syms, std::cout););

  AddUnobservedUnigrams(*align_syms, MDExpectationArc::Weight::One(), result);

  FSTR_CREATE_DBG_EXEC(10,
                       std::cerr << "Trie 2:" << std::endl;
                       util::printTransducer(result, align_syms, align_syms, std::cout););

  Map(result, RmWeightMapper<MDExpectationArc>());
  // TrieInsertFeatures(*align_syms, feature_names, extract_features_fct, "", result);
  timer.stop();
  fprintf(stderr, "Done creating trie [%2.2f ms, %2.2f MB]\n",
          timer.get_elapsed_time_millis(),
          util::MemoryInfo::instance().getSizeInMB());

  const int64 kPhiLabel = -3;
  const int64 kStartLabel = align_syms->Find("S|S");
  const int64 kEndLabel = align_syms->Find("E|E");
  AllSymbols all_align_syms(align_syms, kStartLabel, kEndLabel, kPhiLabel);

  // Creates combined backoff scoring machine, e.g. for target LM and vowel/consonant
  std::vector<Fst<MDExpectationArc>*> backoff_model_fsts;
  backoff_model_fsts.reserve(backoff.size());
  for (std::size_t i = 0; i < backoff.size(); ++i) {
    BackoffSymsFct::Ptr backoff_syms_fct = boost::get<0>(backoff[i]);
    features::ExtractFeaturesFct::Ptr backoff_feats_fct = boost::get<1>(backoff[i]);
    std::size_t backoff_ngram_order = boost::get<2>(backoff[i]);
    MutableFst<MDExpectationArc>* backoff_model_fst = new VectorFst<MDExpectationArc>();
    fstrain::create::CreateBackoffModel(*result,
                                        all_align_syms,
                                        feature_names,
                                        *backoff_syms_fct,
                                        *backoff_feats_fct,
                                        backoff_ngram_order,
                                        *wellformed_fst,
                                        backoff_model_fst);
    backoff_model_fsts.push_back(backoff_model_fst);
  }

  if (max_insertions > -1) {
    MutableFst<MDExpectationArc>* limit_fst = new VectorFst<MDExpectationArc>();
    getLimitMachine(limit_fst, align_syms, max_insertions);
    limit_fst->SetInputSymbols(NULL);
    limit_fst->SetOutputSymbols(NULL);
    backoff_model_fsts.push_back(limit_fst);
  }

  MutableFst<MDExpectationArc>* combined_backoff_model = NULL;
  if (!backoff_model_fsts.empty()) {
    fprintf(stderr, "Adding backoff / insertion limit [%2.2f MB]\n",
            util::MemoryInfo::instance().getSizeInMB());
    combined_backoff_model = new VectorFst<MDExpectationArc>();
    util::Intersect_vec(backoff_model_fsts, combined_backoff_model, false);
    BOOST_FOREACH(Fst<MDExpectationArc>* fst, backoff_model_fsts) {
      delete fst;
    }
    ArcSort(combined_backoff_model, ILabelCompare<MDExpectationArc>());
  }

  FSTR_CREATE_DBG_EXEC(10,
                       std::cerr << "FEATURES:" << std::endl;
                       for (SymbolTableIterator sit(*feature_names); !sit.Done(); sit.Next()) {
                         std::cerr << sit.Value() << " " << sit.Symbol() << std::endl;
                       });

  util::Timer fst_timer;

  NgramFsaInsertFeaturesFct insert_feats_fct(*align_syms, feature_names, extract_features_fct, "");

  // CreateScoringFstFromTrie(result, all_align_syms, *isymbols, *osymbols, ngram_order,
  CreateScoringFstFromTrie(result, all_align_syms, *align_syms, *isymbols, *osymbols, ngram_order,
                           proj_up,
                           proj_down,
                           wellformed_fst,
                           insert_feats_fct,
                           combined_backoff_model);
  delete combined_backoff_model;
  delete align_syms;
  ArcSort(result, ILabelCompare<fst::MDExpectationArc>());

  fst_timer.stop();
  fprintf(stderr, "Done converting to model FST [%2.2f ms, %2.2f MB]\n",
          fst_timer.get_elapsed_time_millis(),
          util::MemoryInfo::instance().getSizeInMB());
}

} } // end namespaces
