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

#include <iostream>
#include <stdexcept>
#include <set>

#include <boost/program_options.hpp>

// #include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/selective-phi-matcher.h"

#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fst/fst.h"
#include "fst/compose.h"
#include "fst/matcher.h"
#include "fst/symbol-table.h"
#include "fst/arcsort.h"

namespace po = boost::program_options;

using namespace fst;

void CreateAllSymsFst(const fst::SymbolTable& syms,
                      fst::MutableFst<LogArc>* result) {
  typedef LogArc::StateId StateId;
  typedef LogArc::Weight Weight;
  StateId s = result->AddState();
  result->SetStart(s);
  const int64 kPhiLabel = syms.Find("phi");
  SymbolTableIterator it(syms);
  it.Next(); // ignore eps
  for(; !it.Done(); it.Next()) {
    if(it.Value() != kPhiLabel) {
      result->AddArc(s, LogArc(it.Value(), it.Value(), Weight::One(), s));
    }
  }
  result->SetFinal(s, Weight::One());
}

void Test(const Fst<LogArc>& fst,
          const SymbolTable& syms) {

  const int64 kPhiLabel = syms.Find("phi");

  VectorFst<LogArc> all_syms_fst;
  CreateAllSymsFst(syms, &all_syms_fst);

  ArcSortFst<LogArc, OLabelCompare<LogArc> > sorted(fst, OLabelCompare<LogArc>());

  // TEST
  std::set<LogArc::Label> phi_match_syms;
  phi_match_syms.insert(syms.Find("a|x"));
  phi_match_syms.insert(syms.Find("a|y"));
  phi_match_syms.insert(syms.Find("b|x"));
  phi_match_syms.insert(syms.Find("b|y"));

  typedef Fst<LogArc> FST;
  typedef Matcher<FST> M;
  typedef SelectivePhiMatcher<M> PM;
  ComposeFstOptions<LogArc, PM> copts;
  copts.gc_limit = 0;
  copts.matcher1 = new PM(sorted, MATCH_OUTPUT, kPhiLabel);
  copts.matcher1->SetPhiMatchingSymbols(phi_match_syms.begin(),
                                        phi_match_syms.end());
  copts.matcher2 = new PM(all_syms_fst, MATCH_NONE);
  ComposeFst<LogArc> composed(sorted, all_syms_fst, copts);
  std::cout << "Composed:" << std::endl;
  fstrain::util::printAcceptor(&composed, &syms, std::cout);
}

int main(int ac, char** av) {
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("help", "produce help message")
        ("symbols", po::value<std::string>(), "symbol table for align syms")
        ("fst", po::value<std::string>(), "FST that contains phi syms")
        ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value<std::string>(), "input file")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    //po::options_description config_file_options;
    //config_file_options.add(generic).add(hidden);

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    store(po::command_line_parser(ac, av).
          options(cmdline_options).positional(p).run(), vm);

    if (vm.count("help")) {
      std::cout << generic << "\n";
      return EXIT_FAILURE;
    }

    if (vm.count("symbols") == 0) {
      std::cerr << "Please specify symbol table with --symbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("fst") == 0) {
      std::cerr << "Please specify phi FST with --fst" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* syms = SymbolTable::ReadText(vm["symbols"].as<std::string>());

    const std::string fst_filename = vm["fst"].as<std::string>();
    MutableFst<LogArc>* the_fst = VectorFst<LogArc>::Read(fst_filename);

    Test(*the_fst, *syms);

    delete syms;
    delete the_fst;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
