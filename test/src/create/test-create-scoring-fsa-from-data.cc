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

#include <boost/program_options.hpp>

#include "fstrain/util/data.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/timer.h"
#include "fstrain/util/memory-info.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/print-fst.h"

#include "fstrain/create/v2/create-scoring-fsa-from-data.h"

#include "fst/vector-fst.h"
#include "fst/fst.h"
#include "fst/compose.h"
#include "fst/arcsort.h"
#include "fst/matcher.h"
#include "fst/symbol-table.h"


namespace po = boost::program_options;

using namespace fst;

int main(int ac, char** av) {
  try{    

    po::options_description generic("Allowed options");
    generic.add_options()
        ("help", "produce help message")
        ("isymbols", po::value<std::string>(), "symbol table for input words")
        ("osymbols", po::value<std::string>(), "symbol table for output words")
        ("align-fst", po::value<std::string>(), "simple alignment FST")
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

    if (vm.count("isymbols") == 0) {
      std::cerr << "Please specify input symbol table with --isymbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("osymbols") == 0) {
      std::cerr << "Please specify output symbol table with --osymbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("align-fst") == 0) {
      std::cerr << "Please specify alignment FST with --align-fst" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("input-file") == 0) {
      std::cerr << "Please give input file as argument" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* isymbols = SymbolTable::ReadText(vm["isymbols"].as<std::string>());
    SymbolTable* osymbols = SymbolTable::ReadText(vm["osymbols"].as<std::string>());

    const std::string data_filename = vm["input-file"].as<std::string>();
    const std::string fst_filename = vm["align-fst"].as<std::string>();

    fstrain::util::Data data(data_filename);
    const Fst<StdArc>* align_fst = fstrain::util::GetVectorFst<StdArc>(fst_filename);

    typedef MDExpectationArc Arc;
    
    const int ngram_order = 3;
    const char sep_char = '|';
    bool add_identity_chars = true; // always add a|a, b|b, ...
    VectorFst<Arc> scoring_fsa;
    VectorFst<Arc> proj_up;
    VectorFst<Arc> proj_down;
    SymbolTable pruned_align_syms("pruned-align-syms");
    SymbolTable feature_names("feature-names");
    fstrain::create::features::ExtractFeaturesFct_Simple extract_features_fct;
    fstrain::util::Timer timer;    
    fstrain::create::CreateScoringFsaFromData(data, *isymbols, *osymbols,
                                              *align_fst, 
                                              ngram_order,
                                              extract_features_fct,
                                              sep_char,
                                              &pruned_align_syms,
                                              add_identity_chars,
                                              &feature_names,
                                              &proj_up, &proj_down,
                                              &scoring_fsa);
    timer.stop();
    fprintf(stderr, "Done creating scoring fsa [%2.2f ms, %2.2f MB]\n", 
            timer.get_elapsed_time_millis(), 
            fstrain::util::MemoryInfo::instance().getSizeInMB());

    const int64 kPhiLabel = pruned_align_syms.Find("phi");
    if(kPhiLabel == kNoLabel){
      throw std::runtime_error("phi not found");      
    }

    ArcSort(&proj_up, ILabelCompare<Arc>());
    ArcSort(&scoring_fsa, ILabelCompare<Arc>());
    for(fstrain::util::Data::const_iterator it = data.begin(); it != data.end(); ++it){
      VectorFst<Arc> input;
      VectorFst<Arc> output;
      fstrain::util::ConvertStringToFst(it->first, *isymbols, &input);
      fstrain::util::ConvertStringToFst(it->second, *osymbols, &output);
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
      fstrain::util::printTransducer(&numerator, isymbols, &pruned_align_syms, std::cout);      
    }
    
    delete isymbols;
    delete osymbols;
    delete align_fst;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
