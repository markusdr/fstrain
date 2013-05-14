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

#include "fst/fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/noepshist/noepshist-create-scoring-fst.h"
#include "fstrain/create/noepshist/backoff-iterator.h"
#include "fstrain/create/noepshist/history-filter.h"
#include "fstrain/util/print-fst.h"

BOOST_AUTO_TEST_SUITE(noepshist)

BOOST_AUTO_TEST_CASE( BackoffIterator_1a ) {   
  using namespace fstrain::create::noepshist;
  const std::string str = "abc|xyz";
  std::set<std::string> want;
  BackoffIterator it(str);
  want.insert("abc|xyz");
  want.insert("bc|xyz");
  want.insert("abc|yz");
  want.insert("c|xyz");
  want.insert("bc|yz");
  want.insert("abc|z");
  want.insert("|xyz");
  want.insert("c|yz");
  want.insert("bc|z");
  want.insert("abc|");
  want.insert("|yz"); 
  want.insert("c|z");
  want.insert("bc|");
  want.insert("|z");
  want.insert("c|");
  want.insert("|");
  int cnt = 0;
  for(int i = 0; i < want.size(); ++i){
    std::string result = it.Value();
    bool result_ok = want.find(result) != want.end();
    BOOST_CHECK_EQUAL(result_ok, true);    
    it.Next();
    ++cnt;
  }
  BOOST_CHECK_EQUAL(it.Done(), true);
}

BOOST_AUTO_TEST_CASE( BackoffIterator_1b ) {   
  using namespace fstrain::create::noepshist;
  const std::string str = "abc|xyz";
  BackoffIterator it(str);
  BOOST_CHECK_EQUAL(it.Value(), "abc|xyz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "bc|xyz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "abc|yz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "c|xyz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "bc|yz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "abc|z");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|xyz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "c|yz");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "bc|z");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "abc|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|yz"); 
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "c|z");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "bc|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|z");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "c|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Done(), true);
}

BOOST_AUTO_TEST_CASE( BackoffIterator_2 ) {
  using namespace fstrain::create::noepshist;
  const std::string str = "|";
  BackoffIterator it(str);
  BOOST_CHECK_EQUAL(it.Value(), "|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Done(), true);
}

BOOST_AUTO_TEST_CASE( BackoffIterator_3 ) {
  using namespace fstrain::create::noepshist;
  const std::string str = "ab|y";
  BackoffIterator it(str);
  BOOST_CHECK_EQUAL(it.Value(), "ab|y");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "b|y");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "ab|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|y");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "b|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Value(), "|");
  it.Next();
  BOOST_CHECK_EQUAL(it.Done(), true);
}

BOOST_AUTO_TEST_CASE( GetBackoffHistory_1 ) {
  using fstrain::create::noepshist::GetBackoffHistory;
  using fstrain::create::noepshist::HistoryFilter_Length;
  const char sep_char = '|';
  HistoryFilter_Length hist_filter(2, 2, sep_char);
  const std::string hist = GetBackoffHistory("S|S", &hist_filter, sep_char);
  BOOST_CHECK_EQUAL( hist, "S|S");
}

BOOST_AUTO_TEST_CASE( GetBackoffHistory_2 ) {
  using fstrain::create::noepshist::GetBackoffHistory;
  using fstrain::create::noepshist::HistoryFilter_Length;
  const char sep_char = '|';
  HistoryFilter_Length hist_filter(0, 0, sep_char);
  const std::string hist = GetBackoffHistory("S|S", &hist_filter, sep_char);
  BOOST_CHECK_EQUAL( hist, "|");
}

BOOST_AUTO_TEST_CASE( GetBackoffHistory_3 ) {
  using fstrain::create::noepshist::GetBackoffHistory;
  using fstrain::create::noepshist::HistoryFilter_Length;
  const char sep_char = '|';
  HistoryFilter_Length hist_filter(2, 2, sep_char);
  const std::string hist = GetBackoffHistory("Sab|Sxy", &hist_filter, sep_char);
  BOOST_CHECK_EQUAL( hist, "ab|xy");
}

BOOST_AUTO_TEST_CASE( GetBackoffHistory_4 ) {
  using fstrain::create::noepshist::GetBackoffHistory;
  using fstrain::create::noepshist::HistoryFilter_Length;
  const char sep_char = '|';
  HistoryFilter_Length hist_filter(2, 2, sep_char);
  const std::string hist = GetBackoffHistory("Sa|xyz", &hist_filter, sep_char);
  BOOST_CHECK_EQUAL( hist, "Sa|yz");
}

BOOST_AUTO_TEST_CASE( ExtendHistory_1 ) {
  using fstrain::create::noepshist::ExtendHistory;
  const std::string hist = ExtendHistory("abc|xyz", "d|z", '|', '-');
  BOOST_CHECK_EQUAL( hist, "abcd|xyzz");
}
BOOST_AUTO_TEST_CASE( ExtendHistory_2 ) {
  using fstrain::create::noepshist::ExtendHistory;
  const std::string hist = ExtendHistory("abc|xyz", "d|-", '|', '-');
  BOOST_CHECK_EQUAL( hist, "abcd|xyz");
}
BOOST_AUTO_TEST_CASE( ExtendHistory_3 ) {
  using fstrain::create::noepshist::ExtendHistory;
  const std::string hist = ExtendHistory("abc|wxy", "-|z", '|', '-');
  BOOST_CHECK_EQUAL( hist, "abc|wxyz");
}
BOOST_AUTO_TEST_CASE( ExtendHistory_4 ) {
  using fstrain::create::noepshist::ExtendHistory;
  const std::string hist = ExtendHistory("|", "a|-", '|', '-');
  BOOST_CHECK_EQUAL( hist, "a|");
}
BOOST_AUTO_TEST_CASE( ExtendHistory_5 ) {
  using fstrain::create::noepshist::ExtendHistory;
  const std::string hist = ExtendHistory("a|", "b|-", '|', '-');
  BOOST_CHECK_EQUAL( hist, "ab|");
}

BOOST_AUTO_TEST_CASE( CreateScoringFst ) {   
  using namespace fstrain::create::noepshist;
  using namespace fst;
  std::cerr << "Creating scoring machine " << std::endl;
  SymbolTable align_syms("align-sym");  
  // align_syms.AddSymbol("-|-");
  align_syms.AddSymbol("a|x");
  align_syms.AddSymbol("a|y");
  align_syms.AddSymbol("b|x");
  align_syms.AddSymbol("b|y");
  align_syms.AddSymbol("-|x");
  align_syms.AddSymbol("-|y");
  align_syms.AddSymbol("a|-");
  align_syms.AddSymbol("b|-");
  const std::string end_sym = "E|E";
  align_syms.AddSymbol(end_sym);
  ScoringFstBuilder<fst::MDExpectationArc> sfb;
  VectorFst<MDExpectationArc> fst;
  SymbolTable features("features");
  const char sep_char = '|';
  HistoryFilter_Length hist_filter(1,1, sep_char);
  sfb.CreateScoringFst(align_syms, end_sym, 
                       &hist_filter, &features, &fst);
  fstrain::util::printAcceptor(&fst, &align_syms, std::cerr);
  std::cerr << "FEATURES:" << std::endl;
  for(SymbolTableIterator sit(features); !sit.Done(); sit.Next()){
    std::cerr << sit.Value() << "\t" << sit.Symbol() << std::endl;
  }
}

BOOST_AUTO_TEST_CASE( CreateScoringFst_2 ) {   
  using namespace fstrain::create::noepshist;
  using namespace fst;
  struct MyHistoryFilter : public HistoryFilter {
    bool operator()(const std::string& h) {
      return h == "E|E" 
          || h == "a|x"
          || h == "|";        
    }
  };
  SymbolTable align_syms("align-sym");
  align_syms.AddSymbol("-|-");
  align_syms.AddSymbol("a|-");
  align_syms.AddSymbol("-|x");
  align_syms.AddSymbol("a|x");
  const std::string end_sym = "E|E";
  align_syms.AddSymbol(end_sym);
  ScoringFstBuilder<fst::MDExpectationArc> sfb;
  VectorFst<MDExpectationArc> fst;
  SymbolTable features("features");
  MyHistoryFilter hist_filter;
  int maxdiff = 1;
  HistoryFilter_LengthDiff feat_filter(maxdiff, '|');
  sfb.SetFeaturesFilter(&feat_filter);
  sfb.CreateScoringFst(align_syms, end_sym, 
                       &hist_filter, &features, &fst);
  fstrain::util::printAcceptor(&fst, &align_syms, std::cerr);
  // BOOST_CHECK_EQUAL(allowed_hists.NumSymbols(), fst.NumStates());
  std::cerr << "FEATURES:" << std::endl;
  for(SymbolTableIterator sit(features); !sit.Done(); sit.Next()){
    std::cerr << sit.Value() << "\t" << sit.Symbol() << std::endl;
  }
}


BOOST_AUTO_TEST_SUITE_END()


