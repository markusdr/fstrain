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

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fstrain/util/print-fst.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/util/compose-fcts.h"
#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fst/arcsort.h"
#include "fst/matcher.h"
#include "fst/compose.h"
#include "fst/invert.h"
#include "fst/determinize.h"
#include "fst/project.h"
#include "fst/map.h"
#include <cstddef> // size_t
#include <set>
#include "fstrain/create/trie-insert-features.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/add-backoff.h"
#include "fstrain/create/backoff-symbols.h"

#include "fstrain/create/create-backoff-model.h"

using namespace fst;

template<class Arc>
void AddArc(int64 from, int64 to, int64 sym, MutableFst<Arc>* fst) {
  fst->AddArc(from,
              Arc(sym, sym, Arc::Weight::One(), to));
}

BOOST_AUTO_TEST_SUITE(create3)

    BOOST_AUTO_TEST_CASE( MyTestCase ) {

  SymbolTable isyms("isyms");
  isyms.AddSymbol("-");
  isyms.AddSymbol("a");
  isyms.AddSymbol("b");
  isyms.AddSymbol("c");

  isyms.AddSymbol("START");
  isyms.AddSymbol("END");
  isyms.AddSymbol("phi");

  SymbolTable osyms("osyms");
  osyms.AddSymbol("-");
  osyms.AddSymbol("x");
  osyms.AddSymbol("y");
  osyms.AddSymbol("z");

  osyms.AddSymbol("START");
  osyms.AddSymbol("END");
  osyms.AddSymbol("phi");

  SymbolTable align_syms("align_syms");
  align_syms.AddSymbol("eps");

  align_syms.AddSymbol("a|-");
  align_syms.AddSymbol("a|x");
  align_syms.AddSymbol("a|y");
  align_syms.AddSymbol("a|z");

  align_syms.AddSymbol("b|-");
  align_syms.AddSymbol("b|x");
  align_syms.AddSymbol("b|y");
  align_syms.AddSymbol("b|z");

  align_syms.AddSymbol("c|-");
  align_syms.AddSymbol("c|x");
  align_syms.AddSymbol("c|y");
  align_syms.AddSymbol("c|z");

  align_syms.AddSymbol("-|x");
  align_syms.AddSymbol("-|y");
  align_syms.AddSymbol("-|z");

  int64 kStartLabel = align_syms.AddSymbol("S|S");
  int64 kEndLabel = align_syms.AddSymbol("E|E");
  int64 kPhiLabel = align_syms.AddSymbol("phi");

  fstrain::create::AllSymbols all_align_syms(&align_syms, kStartLabel, kEndLabel, kPhiLabel);

  // typedef StdArc Arc;
  typedef fst::MDExpectationArc Arc;
  VectorFst<Arc> trie;

  for(int i = 0; i < 11; ++i){
    trie.AddState();
    trie.SetFinal(i, Arc::Weight::One());
  }
  trie.SetStart(0);

  AddArc(0, 1, kStartLabel, &trie);
  AddArc(0, 2, align_syms.Find("a|-"), &trie);
  AddArc(0, 3, align_syms.Find("b|x"), &trie);
  AddArc(0, 4, kEndLabel, &trie);

  // test
  AddArc(0, 10, align_syms.Find("a|z"), &trie);

  AddArc(1, 5, align_syms.Find("a|-"), &trie);

  AddArc(2, 6, align_syms.Find("b|x"), &trie);

  AddArc(3, 7, kEndLabel, &trie);

  AddArc(5, 8, align_syms.Find("b|x"), &trie);

  AddArc(6, 9, kEndLabel, &trie);

  std::cerr << "TRIE:" << std::endl;
  fstrain::util::printTransducer(&trie, &align_syms, &align_syms, std::cerr);

  SymbolTable feature_ids("feats");
  fstrain::create::features::ExtractFeaturesFct_Simple feat_extract_fct;
  fstrain::create::TrieInsertFeatures(align_syms,
                                      &feature_ids,
                                      feat_extract_fct,
                                      "",
                                      &trie);

  // fstrain::create::BackoffSyms_TLM backoff_syms_fct;
  fstrain::create::BackoffSyms_tlm backoff_syms_fct;
  // fstrain::create::features::ExtractFeaturesFct_Simple extract_feats_fct(backoff_syms_fct.GetName());
  fstrain::create::features::ExtractFeaturesFct_Simple extract_feats_fct;
  VectorFst<Arc> backoff_model;
  // const std::size_t ngram_order = 3;

//  fstrain::create::CreateBackoffModel(trie,
//                                      all_align_syms,
//                                      &feature_ids,
//                                      backoff_syms_fct,
//                                      extract_feats_fct,
//                                      ngram_order,
//                                      &backoff_model);

  std::cerr << "TRIE (2):" << std::endl;
  fstrain::util::printTransducer(&trie, &align_syms, &align_syms, std::cerr);

  fstrain::create::ConvertTrieToModel(kStartLabel, kEndLabel, kPhiLabel, &trie);
  const Fst<Arc>& model = trie; // just renaming ...

  std::cerr << "Model:" << std::endl;
  fstrain::util::printTransducer(&model, &align_syms, &align_syms, std::cerr);

  std::cerr << "Backoff Model:" << std::endl;
  fstrain::util::printTransducer(&backoff_model, &align_syms, &align_syms, std::cerr);


  fstrain::util::ComposePhiRightFct<Arc> compose_fct(kPhiLabel);
  VectorFst<Arc> final;
  compose_fct(backoff_model, model, &final);
  Project(&final, PROJECT_OUTPUT);

  std::cerr << "Final:" << std::endl;
  fstrain::util::printTransducer(&final, &align_syms, &align_syms, std::cerr);

  std::cerr << "Features:" << std::endl;
  for(SymbolTableIterator sit(feature_ids); !sit.Done(); sit.Next()) {
    std::cerr << sit.Value() << " " << sit.Symbol() << std::endl;
  }
}


BOOST_AUTO_TEST_SUITE_END()


