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
#ifndef FSTRAIN_CREATE_SCORING_FST_FROM_TRIE_H
#define FSTRAIN_CREATE_SCORING_FST_FROM_TRIE_H

#include <iostream>
#include <string>
#include "fstrain/util/symbol-table-mapper.h"
#include "fstrain/util/memory-info.h"
#include "fst/fst.h"
#include "fst/matcher.h"
#include "fst/compose.h"
#include "fst/arcsort.h"
#include "fst/symbol-table.h"

#include "fstrain/util/print-fst.h"
#include "fstrain/util/remove-weights-keep-features-mapper.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/add-backoff.h"
#include "fstrain/create/debug.h"
#include "fstrain/create/all-symbols.h"
#include "fstrain/util/selective-phi-matcher.h"

namespace fstrain { namespace create {

/**
 * @brief Converts trie to scoring transducer; uses NgramCounter.
 *
 * @todo Rewrite. Should not expand all phi arcs.
 */
template<class Arc, class InsertFeaturesFct>
void CreateScoringFstFromTrie(fst::MutableFst<Arc>* ngram_trie,
                              const AllSymbols& all_align_syms,
                              const fst::SymbolTable& essential_align_syms,
                              const fst::SymbolTable& isymbols,
                              const fst::SymbolTable& osymbols,
                              const std::size_t ngram_order,
                              const fst::Fst<fst::StdArc>* proj_up,
                              const fst::Fst<fst::StdArc>* proj_down,
                              const fst::Fst<fst::StdArc>* wellformed_fsa,
                              InsertFeaturesFct& insert_feats_fct,
                              fst::Fst<Arc>* other_scoring_fsa = NULL) {
  using namespace fst;
  typedef typename Arc::Weight Weight;

  const SymbolTable& align_syms = *all_align_syms.symbol_table;
  FSTR_CREATE_DBG_EXEC(10,
                       std::cerr << "CreateScoringFstFromTrie called with:" << std::endl;
                       util::printAcceptor(ngram_trie, &align_syms, std::cerr);
                       std::cerr << std::endl;);

  std::cerr << "Making model" << std::endl;
  ConvertTrieToModel(all_align_syms.kStartLabel, all_align_syms.kEndLabel, all_align_syms.kPhiLabel,
                     ngram_trie);

  typedef fst::WeightConvertMapper<fst::StdArc, fst::MDExpectationArc> Map_SE;
  if (wellformed_fsa == NULL) {
    FSTR_CREATE_EXCEPTION("Currently, wellformed should be there");
  }
  if (wellformed_fsa != NULL) {
    FSTR_CREATE_DBG_EXEC(10, std::cerr << "WELLFORMED:" << std::endl;
                         util::printTransducer(wellformed_fsa,
                                               wellformed_fsa->InputSymbols(),
                                               wellformed_fsa->OutputSymbols(),
                                               std::cerr););
    fst::MapFst<fst::StdArc, fst::MDExpectationArc, Map_SE> mapped(*wellformed_fsa, Map_SE());
    ngram_trie->SetInputSymbols(&align_syms);// to match wellformed_fsa
    ngram_trie->SetOutputSymbols(&align_syms);// to match wellformed_fsa
    fst::ArcSort(ngram_trie, fst::OLabelCompare<fst::MDExpectationArc>());
    typedef fst::Fst<fst::MDExpectationArc> FST;
    typedef fst::Matcher<FST> M;
    typedef fst::PhiMatcher<M> PM;
    typedef fst::SelectivePhiMatcher<M> SPM;
    SPM::LabelSet essential_align_syms_set;
    for (fst::SymbolTableIterator it(essential_align_syms); !it.Done(); it.Next()) {
      essential_align_syms_set.insert(it.Value());
    }
    fst::ComposeFstOptions<fst::MDExpectationArc, SPM> copts;
    copts.gc_limit = 0;
    copts.matcher1 = new SPM(*ngram_trie, fst::MATCH_OUTPUT, all_align_syms.kPhiLabel);
    copts.matcher1->SetPhiMatchingSymbols(essential_align_syms_set.begin(),
                                          essential_align_syms_set.end());
    copts.matcher2 = new SPM(mapped, fst::MATCH_NONE);
    *ngram_trie = fst::ComposeFst<fst::MDExpectationArc>(*ngram_trie, mapped, copts);
    insert_feats_fct(ngram_trie);
    ngram_trie->SetInputSymbols(NULL);
    ngram_trie->SetOutputSymbols(NULL);
  }

  if (other_scoring_fsa != NULL) {
    fprintf(stderr, "Adding FSAs (backoffs, insertion limit, etc.) [%2.2f MB]\n",
            util::MemoryInfo::instance().getSizeInMB());
    FSTR_CREATE_DBG_EXEC(10,
                         std::cerr << "ngram_trie:" << std::endl;
                         util::printTransducer(ngram_trie, &align_syms, &align_syms, std::cerr);
                         std::cerr << "other_scoring_fsa:" << std::endl;
                         util::printTransducer(other_scoring_fsa, &align_syms, &align_syms, std::cerr););

    *ngram_trie = ComposeFst<Arc>(*ngram_trie, *other_scoring_fsa);

    FSTR_CREATE_DBG_EXEC(10,
                         std::cerr << "Composition:" << std::endl;
                         util::printTransducer(ngram_trie, &align_syms, &align_syms, std::cerr););
    fprintf(stderr, "Done adding FSAs (backoffs, insertion limit, etc.) [%2.2f MB]\n",
            util::MemoryInfo::instance().getSizeInMB());
  }

  fst::ComposeFstOptions<fst::MDExpectationArc> copts2;
  copts2.gc_limit = 0;

  if (proj_up != NULL) {
    FSTR_CREATE_DBG_EXEC(10, std::cerr << "PROJ_UP:" << std::endl;
                         util::printTransducer(proj_up,
                                               proj_up->InputSymbols(),
                                               proj_up->OutputSymbols(),
                                               std::cerr););
    typedef util::SymbolTableMapper<fst::StdArc> SymMapper;
    SymMapper sym_mapper(*proj_up->InputSymbols(), isymbols,
                         *proj_up->OutputSymbols(), align_syms);
    MapFst<fst::StdArc, fst::StdArc, SymMapper> mapped1(*proj_up, sym_mapper);
    MapFst<fst::StdArc, fst::MDExpectationArc, Map_SE> mapped2(mapped1, Map_SE());
    *ngram_trie = fst::ComposeFst<fst::MDExpectationArc>(mapped2, *ngram_trie, copts2);
  }

  if (proj_down != NULL) {
    FSTR_CREATE_DBG_EXEC(10, std::cerr << "PROJ_DOWN:" << std::endl;
                         util::printTransducer(proj_down,
                                               proj_down->InputSymbols(),
                                               proj_down->OutputSymbols(),
                                               std::cerr););
    typedef util::SymbolTableMapper<fst::StdArc> SymMapper;
    SymMapper sym_mapper(*proj_down->InputSymbols(), align_syms,
                         *proj_down->OutputSymbols(), osymbols);
    MapFst<fst::StdArc, fst::StdArc, SymMapper> mapped1(*proj_down, sym_mapper);
    MapFst<fst::StdArc, fst::MDExpectationArc, Map_SE> mapped2(mapped1, Map_SE());
    *ngram_trie = fst::ComposeFst<fst::MDExpectationArc>(*ngram_trie, mapped2, copts2);
  }

  Map(ngram_trie, util::RemoveWeightsKeepFeaturesMapper<Arc>());

  FSTR_CREATE_DBG_EXEC(10,
                       std::cerr << "NGRAM_TRIE:" << std::endl;
                       util::printTransducer(ngram_trie,
                                             ngram_trie->InputSymbols(),
                                             ngram_trie->OutputSymbols(),
                                             std::cerr);
                       std::cerr << std::endl;);
}

} } // end namespaces

#endif
