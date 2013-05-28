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
#ifndef FSTRAIN_CREATE_CREATE_BACKOFF_MODEL_H
#define FSTRAIN_CREATE_CREATE_BACKOFF_MODEL_H

#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fst/compose.h"
#include "fst/invert.h"
#include "fst/determinize.h"
#include "fst/rmepsilon.h"
#include "fst/project.h"
#include "fst/map.h"

#include "fstrain/create/add-backoff.h"
#include "fstrain/create/all-symbols.h"
#include "fstrain/create/ngram-counter.h"
#include "fstrain/create/ngram-fsa-insert-features.h"
#include "fstrain/util/compose-fcts.h"
#include "fstrain/util/options.h"
#include "fstrain/util/print-fst.h"

namespace fstrain { namespace create {

/**
 * @brief Creates transducer that converts from alignment symbols to
 * backed-off symbols, e.g. a|x -> x
 *
 * @param backoff_syms_fct A functor that reduces the symbols (e.g. a|x -> x)
 */
template<class Arc, class Fct>
void GetBackoffSymsFst(const AllSymbols& align_syms,
                       Fct& backoff_syms_fct,
                       fst::SymbolTable* result_syms,
                       fst::MutableFst<Arc>* result_fst) {
  using namespace fst;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  result_syms->AddSymbol("eps");
  StateId s = result_fst->AddState();
  result_fst->SetStart(s);
  result_fst->SetFinal(s, Arc::Weight::One());
  SymbolTableIterator sit(*align_syms.symbol_table);
  sit.Next(); // ignore eps
  for(; !sit.Done(); sit.Next()) {
    const int64 old_label = sit.Value();
    int64 new_label;
    const bool is_special_sym =
        old_label == align_syms.kStartLabel
        || old_label == align_syms.kEndLabel
        || old_label == align_syms.kPhiLabel;
    const std::string backoff_sym =
        is_special_sym ? sit.Symbol() : backoff_syms_fct(sit.Symbol());
    new_label = result_syms->AddSymbol(backoff_sym);
    if(sit.Value() != align_syms.kPhiLabel) {
      result_fst->AddArc(s, Arc(sit.Value(), new_label, Weight::One(), s));
    }
  }
}

/**
 * @brief Creates backoff model (i.e. an ngram-like model over backoff
 * symbols), from a trie of alignment symbol observations.
 */
template<class Arc, class BackoffSymsFct, class ExtractFeatsFct>
void CreateBackoffModel(const fst::Fst<Arc>& align_syms_trie,
                        const AllSymbols& align_syms,
                        fst::SymbolTable* feature_ids,
                        BackoffSymsFct& backoff_syms_fct,
                        ExtractFeatsFct& extract_feats_fct,
                        std::size_t ngram_order,
                        const fst::Fst<fst::StdArc>& wellformed_fst,
                        fst::MutableFst<Arc>* result)
{
  using namespace fst;
  VectorFst<Arc> backoff_syms_transducer;
  SymbolTable backoff_syms("backoff");
  GetBackoffSymsFst(align_syms, backoff_syms_fct,
                    &backoff_syms, &backoff_syms_transducer);

  VectorFst<Arc> trie2;
  Compose(align_syms_trie, backoff_syms_transducer, &trie2);
  Project(&trie2, PROJECT_OUTPUT);
  Map(&trie2, RmWeightMapper<Arc>());

  NgramCounter<Arc> ngram_counter(ngram_order);
  ngram_counter.AddCounts(trie2);
  VectorFst<Arc> trie2_det;
  ngram_counter.GetResult(&trie2_det);
  Map(&trie2_det, RmWeightMapper<Arc>());

  const int64 kPhiLabel = backoff_syms.AddSymbol("phi");
  fstrain::create::ConvertTrieToModel(backoff_syms.Find("S|S"),
                                      backoff_syms.Find("E|E"),
                                      kPhiLabel,
                                      &trie2_det);

  typedef WeightConvertMapper<StdArc, Arc> Map_SA;
  MapFst<StdArc, Arc, Map_SA> mapped_wellformed(wellformed_fst, Map_SA());

  fstrain::util::ComposePhiLeftFct<Arc> compose_fct(kPhiLabel);
  // compose_fct(trie2_det, InvertFst<Arc>(backoff_syms_transducer), result);

  fst::VectorFst<Arc> inverted_backoff_syms_transducer = backoff_syms_transducer;
  Invert(&inverted_backoff_syms_transducer);
  inverted_backoff_syms_transducer.SetOutputSymbols(align_syms.symbol_table); // match wellformed
  compose_fct(trie2_det,DeterminizeFst<Arc>(ProjectFst<Arc>(ComposeFst<Arc>(inverted_backoff_syms_transducer,
                                                                            mapped_wellformed),
                                                            PROJECT_INPUT)), result);
  result->SetOutputSymbols(NULL);
  // Project(result, PROJECT_INPUT);
  Map(result, RmWeightMapper<Arc>());

  std::cerr << "# Inserting features into the backoff ngram FSA" << std::endl;
  const std::string prefix = backoff_syms_fct.GetName() + ":"; // e.g. "tlm:"
  fstrain::create::NgramFsaInsertFeatures(backoff_syms,
                                          feature_ids,
                                          extract_feats_fct,
                                          prefix,
                                          result);
  // compose_fct(trie2_det, InvertFst<Arc>(backoff_syms_transducer), result);
  VectorFst<Arc> tmp;
  ArcSort(result, OLabelCompare<Arc>());
  Compose(*result, InvertFst<Arc>(backoff_syms_transducer), &tmp);
  *result = tmp;
  Project(result, PROJECT_OUTPUT);

//  std::cerr << "CreateBackoffModel result:" << std::endl;
//  fstrain::util::printTransducer(result, &backoff_syms, align_syms.symbol_table,
//                                 std::cout);

}

} } // end namespaces

#endif
