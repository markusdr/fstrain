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
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"

#include "fstrain/util/data.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/print-fst.h"

#include "fstrain/create/create-ngram-fst-from-best-align.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/backoff-symbols.h"
#include "fstrain/create/backoff-util.h"
#include "fstrain/create/prune-fct.h"

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace po = boost::program_options;

using namespace fst;
using namespace fstrain;

int main(int ac, char** av){
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("help", "produce help message")
        ("max-insertions", po::value<int>()->default_value(-1), "")
        ("num-conjugations", po::value<int>()->default_value(0), "")
        ("num-change-regions", po::value<int>()->default_value(0), "")
        ("ngram-order", po::value<int>()->default_value(3), "")
        ("align-fst", po::value<std::string>(), "")
        ("data", po::value<std::string>(), "")
        ("isymbols", po::value<std::string>(), "symbol table for input words")
        ("osymbols", po::value<std::string>(), "symbol table for output words")
        ("features", po::value<std::string>()->default_value("simple"), "feature extraction")
        ("backoff", po::value<std::string>()->default_value(""), "backoff, e.g. vc,tlm/add-collapsed")
        ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value< std::string >(), "input file")
        ;
    
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    po::options_description config_file_options;
    config_file_options.add(generic).add(hidden);

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    store(po::command_line_parser(ac, av).
	  options(cmdline_options).positional(p).run(), vm);

    if(vm.count("config-file")){
      ifstream ifs(vm["config-file"].as<std::string>().c_str());
      store(parse_config_file(ifs, config_file_options), vm);
      notify(vm);
    }

    if (vm.count("help")) {
      std::cout << generic << "\n";
      return EXIT_FAILURE;
    }

    if (vm.count("isymbols") == 0) {
      std::cerr << "Please specify symbol table with --isymbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("osymbols") == 0) {
      std::cerr << "Please specify symbol table with --osymbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("data") == 0) {
      std::cerr << "Please use --data" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("align-fst") == 0) {
      std::cerr << "Please alignment fst table with --align-fst" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* isymbols = SymbolTable::ReadText(vm["isymbols"].as<std::string>());
    SymbolTable* osymbols = SymbolTable::ReadText(vm["osymbols"].as<std::string>());    

    const std::string fname = vm["features"].as<std::string>();
    const std::string backoff = vm["backoff"].as<std::string>();

    const std::string data_filename = vm["data"].as<std::string>();
    const std::string align_fst_filename = vm["align-fst"].as<std::string>();

    const int num_conjugations = vm["num-conjugations"].as<int>();
    const int num_change_regions = vm["num-change-regions"].as<int>();
    const int ngram_order = vm["ngram-order"].as<int>();
    
    const Fst<StdArc>* align_fst = util::GetVectorFst<StdArc>(align_fst_filename);

    using namespace fstrain::create;
    using namespace fstrain::create::features;
    ExtractFeaturesFct* extract_features_fct = new ExtractFeaturesFctPlugin(fname);

    std::vector<std::string> backoff_names;
    typedef features::ExtractFeaturesFct::Ptr ExtractFctPtr;
    std::vector< boost::tuple<BackoffSymsFct::Ptr, ExtractFctPtr, std::size_t> > backoff_vec;
    ParseBackoffDescription(backoff, ngram_order, &backoff_vec);

    SymbolTable feature_names("features");

    feature_names.AddSymbol("*LENGTH_IN*");
    feature_names.AddSymbol("*LENGTH_OUT*"); // to give them Ids 0 and 1
        
    const int max_insertions = vm["max-insertions"].as<int>();

    //  void CreateNgramFstFromBestAlign(size_t ngram_order,
    //				   int max_insertions,
    //				   int num_conjugations,
    //				   int num_change_regions,
    //				   const fst::Fst<fst::StdArc>& alignment_fst,
    //				   PruneFct* prune_fct,
    //				   const std::string& data_filename,
    //				   const fst::SymbolTable* isymbols,
    //				   const fst::SymbolTable* osymbols,
    //				   features::ExtractFeaturesFct& extract_features_fct,
    //				   const std::vector< std::pair<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr> >& backoff,
    //				   fst::SymbolTable* feature_names,
    //				   fst::MutableFst<fst::MDExpectationArc>* result);

    PruneFct* prune_fct = NULL;
    prune_fct = new DefaultPruneFct(fst::StdArc::Weight(-log(0.9)), 1000);      
    
    fst::VectorFst<fst::MDExpectationArc> result;
    fstrain::create::CreateNgramFstFromBestAlign(ngram_order, max_insertions, num_conjugations, num_change_regions, 
                                                 *align_fst, prune_fct, data_filename, isymbols, osymbols, 
                                                 *extract_features_fct, backoff_vec, &feature_names, &result);

    std::cerr << "Result:" << std::endl;
    fstrain::util::printTransducer(&result, isymbols, osymbols, std::cout);    
    
    std::cerr << "Features:" << std::endl;
    for(SymbolTableIterator sit(feature_names); !sit.Done(); sit.Next()) {
      std::cout << sit.Value() << " " << sit.Symbol() << std::endl;
    }
    
    delete extract_features_fct;
    delete prune_fct;
    delete align_fst;
    delete isymbols;
    delete osymbols;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
