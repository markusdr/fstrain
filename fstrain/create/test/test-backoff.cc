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

#include "fst/compose.h"
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/mutable-fst.h"
#include "fst/project.h"
#include "fst/arcsort.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/intersect.h"
#include "fst/rmepsilon.h"
#include "fst/shortest-distance.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/backoff-util.h"
#include "fstrain/create/create-backoff-model.h"
#include "fstrain/util/determinize-in-place.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/remove-weights-keep-features-mapper.h"
#include "fstrain/util/string-weight-mapper.h"
// #include "fstrain/create/trie-insert-features.h" // MyFeatureSet
#include "fstrain/create/ngram-fsa-insert-features.h" 

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = boost::program_options;




//struct NgramFsa_AddBackoff_Fct {
//  typedef fstrain::create::BackoffSymsFct::Ptr BSymsFctPtr;
//  typedef fstrain::create::features::ExtractFeaturesFct::Ptr ExtrFeatFctPtr;
//  typedef std::pair<BSymsFctPtr, ExtrFeatFctPtr> SymsAndFeats;
//  const std::vector<SymsAndFeats>& backoff_fcts;
//  NgramFsa_AddBackoff_Fct(const std::vector<SymsAndFeats>& backoff_fcts_) : backoff_fcts(backoff_fcts_) {}
//
//  template<class Arc>
//  void CreateCombinedBackoffFst(const fst::Fst<Arc>& ngram_fst,
//                                const std::size_t ngram_order,
//                                const fst::SymbolTable& syms,
//                                fst::SymbolTable* feature_ids,
//                                fst::MutableFst<Arc>* result) const {
//    using namespace fst;
//    fstrain::create::AllSymbols all_syms(&syms, fst::kNoLabel, fst::kNoLabel, fst::kNoLabel);
//    *result = ngram_fst;
//    ArcSort(result, ILabelCompare<Arc>());
//    BOOST_FOREACH(const SymsAndFeats& sf, backoff_fcts) {
//      BSymsFctPtr backoff_syms_fct = sf.first;
//      ExtrFeatFctPtr extract_feats_fct = sf.second;
//
//      fst::VectorFst<Arc> backoff_syms_transducer;
//      fst::SymbolTable backoff_symtable("backoff-syms");
//      fstrain::create::GetBackoffSymsFst(all_syms, *backoff_syms_fct, 
//                                         &backoff_symtable, &backoff_syms_transducer);
//
//      MutableFst<Arc>* backed_off = new VectorFst<Arc>();
//      Compose(ngram_fst, backoff_syms_transducer, backed_off);
//      Project(backed_off, PROJECT_OUTPUT);
//      fstrain::util::Determinize(backed_off);
//      Map(backed_off, RmWeightMapper<Arc>());
//      
//      fstrain::create::NgramFsaInsertFeatures(ngram_order,
//                                              backoff_symtable,
//                                              feature_ids,
//                                              *extract_feats_fct,
//                                              backoff_syms_fct->GetName() + ":",
//                                              backed_off);
//      MutableFst<Arc>* backed_off2 = new VectorFst<Arc>();
//      Compose(*backed_off, InvertFst<Arc>(backoff_syms_transducer), backed_off2);
//      delete backed_off;
//      backed_off = backed_off2;
//      Project(backed_off2, PROJECT_OUTPUT);      
//
//      ArcSort(backed_off2, ILabelCompare<Arc>());
//
//      FSTR_CREATE_DBG_EXEC(10, std::cerr << "Backed off:" << std::endl;
//                           fstrain::util::printTransducer(backed_off, &syms, &syms, std::cerr););
//
//      VectorFst<Arc> tmp;
//      Overlay(*result, *backed_off, &tmp);
//      *result = tmp;
//
//      delete backed_off;
//    }
//    FSTR_CREATE_DBG_EXEC(10, std::cerr << "Combined backoff:" << std::endl;
//                         fstrain::util::printTransducer(result, &syms, &syms, std::cerr););
//  }
//
//  template<class Arc>
//  void operator()(const fst::Fst<Arc>& ngram_fst, 
//                  const std::size_t ngram_order,
//                  const fst::SymbolTable& syms,
//                  fst::SymbolTable* feature_ids,
//                  fst::MutableFst<Arc>* ofst) const {
//    using namespace fst;
//    fst::VectorFst<Arc> combined_backoff;
//    CreateCombinedBackoffFst(ngram_fst, ngram_order, syms, 
//                             feature_ids, ofst);    
//    // *ofst = DeterminizeFst<Arc>(RmEpsilonFst<Arc>(UnionFst<Arc>(ngram_fst, combined_backoff)));    
//    // *ofst = RmEpsilonFst<Arc>(UnionFst<Arc>(ngram_fst, combined_backoff));
//    //Overlay(ArcSortFst<Arc, ILabelCompare<Arc> >(ngram_fst, ILabelCompare<Arc>()),
//    //      ArcSortFst<Arc, ILabelCompare<Arc> >(combined_backoff, ILabelCompare<Arc>()),
//    //      ofst);
//  }
//
//};

using namespace fst;
// using namespace fstrain;


int main(int ac, char** av){
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("help", "produce help message")
        ("symbols", po::value<std::string>(), "symbol table")
        ("fst", po::value<std::string>(), "tranducer file name for decoding")
        ("backoff", po::value<std::string>()->default_value("vc"), "backoff description")
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

    if (vm.count("symbols") == 0) {
      std::cerr << "Please specify symbol table with --symbols" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("fst") == 0) {
      std::cerr << "Please alignment fst table with --fst" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* symbols = SymbolTable::ReadText(vm["symbols"].as<std::string>());

    const std::string fst_filename = vm["fst"].as<std::string>();
    const std::string backoff = vm["backoff"].as<std::string>();

    MutableFst<MDExpectationArc>* fst = VectorFst<MDExpectationArc>::Read(fst_filename);

    SymbolTable feature_ids("feature-ids");
    const std::string libname = "simple";
    fstrain::create::features::ExtractFeaturesFctPlugin extract_feats_fct(libname);

//    fstrain::create::NgramFsaInsertFeatures(ngram_order,
//                                            *symbols,
//                                            &feature_ids,
//                                            extract_feats_fct,
//                                            "",
//                                            fst);
  
    fstrain::create::NgramFsaInsertFeatures(*symbols, &feature_ids, extract_feats_fct, "", fst);

    const std::size_t ngram_order = 3; // TEST
  
    using namespace fstrain;
    using namespace fstrain::create;
    std::vector<boost::tuple<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr, std::size_t> > backoff_fcts;
    ParseBackoffDescription(backoff, ngram_order, &backoff_fcts);

    VectorFst<MDExpectationArc> result;
    //NgramFsa_AddBackoff_Fct add_backoff_fct(backoff_fcts);
    //add_backoff_fct(*fst, ngram_order, *symbols, &feature_ids, &result);

    std::cerr << "Result:" << std::endl;
    fstrain::util::printTransducer(fst, symbols, symbols, std::cerr);

    std::cerr << "Features:" << std::endl;
    for(SymbolTableIterator sit(feature_ids); !sit.Done(); sit.Next()) {
      std::cerr << sit.Value() << " " << sit.Symbol() << std::endl;
    }
            
    delete fst;
    delete symbols;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
