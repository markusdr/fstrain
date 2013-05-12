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
#include "fstrain/create/v2/get-project-up-down.h"
#include "fstrain/create/v2/create-lattice.h"
#include "fstrain/create/v2/create-ngram-trie-from-data.h"
#include "fstrain/create/v2/concat-start-and-end-labels.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/core/expectation-arc.h"
#include "fst/vector-fst.h"
#include "fst/arcsort.h"
#include "fst/matcher.h"
#include "fst/compose.h"
#include "fst/map.h"
#include "fstrain/util/string-to-fst.h"
#include <cstddef> // size_t?
#include "fstrain/create/trie-insert-features.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/add-backoff.h"

BOOST_AUTO_TEST_SUITE(create2)

    BOOST_AUTO_TEST_CASE( GetProjUpDown ) {   
  using namespace fst;

  SymbolTable isyms("isyms");
  isyms.AddSymbol("eps");
  isyms.AddSymbol("a");
  isyms.AddSymbol("b");
  isyms.AddSymbol("c");

  SymbolTable osyms("osyms");
  osyms.AddSymbol("eps");
  osyms.AddSymbol("x");
  osyms.AddSymbol("y");
  osyms.AddSymbol("z");

  SymbolTable align_syms("align_syms");
  align_syms.AddSymbol("eps");
  align_syms.AddSymbol("a|eps");
  align_syms.AddSymbol("a|x");
  align_syms.AddSymbol("a|y");
  align_syms.AddSymbol("a|z");
  align_syms.AddSymbol("b|eps");
  align_syms.AddSymbol("b|x");
  align_syms.AddSymbol("b|y");
  align_syms.AddSymbol("b|z");
  align_syms.AddSymbol("c|eps");
  align_syms.AddSymbol("c|x");
  align_syms.AddSymbol("c|y");
  align_syms.AddSymbol("c|z");
  align_syms.AddSymbol("eps|x");
  align_syms.AddSymbol("eps|y");
  align_syms.AddSymbol("eps|z");

  align_syms.AddSymbol("START");
  align_syms.AddSymbol("END");
  align_syms.AddSymbol("phi");

  int64 kStartLabel = align_syms.Find("START");
  int64 kEndLabel = align_syms.Find("END");

  typedef MDExpectationArc Arc;
  VectorFst<Arc> proj_up;
  VectorFst<Arc> proj_down;

  const char sep_char = '|';

  fstrain::create::GetProjUpDown(isyms, osyms, align_syms, sep_char, 
                                 &proj_up, &proj_down);
  fstrain::create::ConcatStartAndEndLabels(kStartLabel, kEndLabel, true, false, &proj_up);
  fstrain::create::ConcatStartAndEndLabels(kStartLabel, kEndLabel, false, true, &proj_down);

  std::cout << "UP:" << std::endl;
  fstrain::util::printTransducer(&proj_up, &isyms, &align_syms, std::cout);
  std::cout << "DOWN:" << std::endl;
  fstrain::util::printTransducer(&proj_down, &align_syms, &osyms, std::cout);

  const std::string in_str("a b");
  const std::string out_str("x y");
  VectorFst<Arc> input;
  VectorFst<Arc> output;
  fstrain::util::ConvertStringToFst(in_str, isyms, &input);
  fstrain::util::ConvertStringToFst(out_str, osyms, &output);
  std::cout << "INPUT:" << std::endl;
  fstrain::util::printAcceptor(&input, &isyms, std::cout);
  std::cout << "OUTPUT:" << std::endl;
  fstrain::util::printAcceptor(&output, &osyms, std::cout);

  VectorFst<Arc> lattice;
  fstrain::create::CreateLattice(input, output, proj_up, proj_down, &lattice);

  std::cout << "LATTICE:" << std::endl;
  fstrain::util::printTransducer(&lattice, &align_syms, &align_syms, std::cout);

  fstrain::util::Data data;
  data.push_back(in_str, out_str);
  std::string s1("b");
  std::string s2("y");
  data.push_back(s1, s2);

  size_t ngram_order = 3;
  VectorFst<Arc> scoring_fsa;
  fstrain::create::CreateNgramTrieFromData(data, 
                                           isyms, osyms, 
                                           proj_up, proj_down, 
                                           align_syms,
                                           ngram_order, 
                                           &scoring_fsa);
  int64 kPhiLabel = align_syms.Find("phi");
  SymbolTable feature_names("feature_names");
  fstrain::create::features::ExtractFeaturesFct_Simple extract_features_fct;
  fst::Map(&scoring_fsa, fst::RmWeightMapper<Arc>());
  fstrain::create::TrieInsertFeatures(align_syms, &feature_names, extract_features_fct, "",
                                      &scoring_fsa);
  fstrain::create::ConvertTrieToModel(kStartLabel, kEndLabel, kPhiLabel, &scoring_fsa);

  ArcSort<Arc, ILabelCompare<Arc> >(&scoring_fsa, ILabelCompare<Arc>());
  std::cout << "SCORING_FSA:" << std::endl;
  fstrain::util::printAcceptor(&scoring_fsa, &align_syms, std::cout);

  std::cout << "FEATURES:" << std::endl;
  for(SymbolTableIterator sit(feature_names); !sit.Done(); sit.Next()) {
    std::cout << sit.Symbol() << " " << sit.Value() << std::endl;
  }

  // This is how we will compose in training:
  VectorFst<Arc> in_up;
  Compose(input, proj_up, &in_up);
  typedef Fst<Arc> FST;
  typedef PhiMatcher< SortedMatcher<FST> > SM;
  ComposeFstOptions<Arc, SM> opts;
  // const bool rewrite_both = true;
  opts.gc_limit = 0;
  opts.matcher1 = new SM(in_up, MATCH_NONE);
  opts.matcher2 = new SM(scoring_fsa, MATCH_INPUT, kPhiLabel);
  ComposeFst<Arc> numerator(in_up, scoring_fsa, opts);

  std::cout << "NUMERATOR:" << std::endl;
  fstrain::util::printTransducer(&numerator, &isyms, &align_syms, std::cout);
  // fstrain::util::printTransducer(&numerator, NULL, NULL, std::cout);
  
}


BOOST_AUTO_TEST_SUITE_END()


