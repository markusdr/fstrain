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
#include "fst/shortest-path.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"

#include "fstrain/util/approx-determinize.h"
#include "fstrain/util/data.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/print-path.h"
#include "fstrain/util/string-to-fst.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>

namespace po = boost::program_options;

using namespace fst;
using namespace fstrain;

class MultipleAnswersCompare {
  std::string separator_;
 public:
  MultipleAnswersCompare(const std::string& sep) 
      : separator_(sep) {}
  bool operator()(const std::string& hypothesis, 
                  const std::string& truths) const {
    std::list<std::string> tok_list;
    boost::iter_split(tok_list, truths, boost::first_finder(separator_));
    BOOST_FOREACH(std::string token, tok_list) {
      if(hypothesis == token){
        return true;
      }
    }
    return false;
  }
};

struct DecodeDataOptions {
  std::ostream* out;
  const SymbolTable& isymbols;
  const SymbolTable& osymbols;
  size_t kbest_paths;
  bool do_evaluate;
  DecodeDataOptions(const SymbolTable& isymbols_, const SymbolTable& osymbols_)
      : out(&std::cout), isymbols(isymbols_), osymbols(osymbols_), kbest_paths(1000),
        do_evaluate(false)
  {}
};

void PrintTransducerOutput(const Fst<StdArc>& input, const Fst<StdArc>& model, 
			   DecodeDataOptions opts) {
  ComposeFst<StdArc> composed(input, model);
  ProjectFst<StdArc> all_output_paths(composed, PROJECT_OUTPUT);
  VectorFst<StdArc> determinized;
  util::approxDeterminize(all_output_paths, &determinized, opts.kbest_paths, kDelta);
  VectorFst<StdArc> best;
  ShortestPath(determinized, &best, 1);
  util::PrintAcceptorArcFct<StdArc> print_fst_fct(&opts.osymbols, opts.out);
  util::PrintPath(best, print_fst_fct);
  // (*opts.out) << std::endl;
}

template<class EqualFct>
void DecodeData(const util::Data& data, 
		const Fst<StdArc>& model_fst,                
                EqualFct equal_fct,
		DecodeDataOptions opts) {
  int num_correct = 0;
  for(util::Data::const_iterator it = data.begin(); it != data.end(); ++it){
    const std::string input_string = it->first;
    VectorFst<StdArc> input_fst;
    const bool delete_unknown_chars = true; 
    util::ConvertStringToFst(input_string, opts.isymbols, 
                             &input_fst, delete_unknown_chars);
    if(opts.do_evaluate){
      std::stringstream ss;
      std::ostream* out = opts.out;
      opts.out = &ss;
      try{
	PrintTransducerOutput(input_fst, model_fst, opts);
      } catch(...) {
	ss << "<NO OUTPUT>";
      }
      opts.out = out;
      (*out) << ss.str() << std::endl;
      if(equal_fct(ss.str(), it->second)){
        ++num_correct;
      }
    }
    else {
      PrintTransducerOutput(input_fst, model_fst, opts);
      (*opts.out) << std::endl;
    }
  }
  if(opts.do_evaluate){
    double accuracy = num_correct / (double)data.size();
    fprintf(stderr, "%d / %d = %2.4f correct\n", 
	    num_correct, (int)data.size(), accuracy);
  }
}

int main(int ac, char** av){
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("help", "produce help message")
        ("isymbols", po::value<std::string>(), "symbol table for input words")
        ("osymbols", po::value<std::string>(), "symbol table for output words")
        ("fst", po::value<std::string>(), "tranducer file name for decoding")
        ("multiple-truths-separator", po::value<std::string>()->default_value(" ### "), 
         "multiple truths separator")
        ("evaluate", po::value<bool>()->default_value(true), "evaluate accuracy?")
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

    if (vm.count("fst") == 0) {
      std::cerr << "Please alignment fst table with --fst" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* isymbols = SymbolTable::ReadText(vm["isymbols"].as<std::string>());
    SymbolTable* osymbols = SymbolTable::ReadText(vm["osymbols"].as<std::string>());    

    util::Data* data = 0;
    if (vm.count("input-file") == 0) {
      std::cerr << "Reading from stdin ..." << std::endl;
      data = new util::Data(std::cin);
    }
    else {
      const std::string data_filename = vm["input-file"].as<std::string>();
      data = new util::Data(data_filename);
    }

    const std::string fst_filename = vm["fst"].as<std::string>();
    
    const Fst<StdArc>* fst = util::GetVectorFst<StdArc>(fst_filename);
    DecodeDataOptions opts(*isymbols, *osymbols);
    opts.do_evaluate = vm["evaluate"].as<bool>();    
    const std::string separator = vm["multiple-truths-separator"].as<std::string>();
    if(separator.length()) {
      DecodeData(*data, *fst, MultipleAnswersCompare(separator), opts);
    }
    else {
      DecodeData(*data, *fst, boost::is_equal(), opts);
    }

    delete data;
    delete fst;
    delete isymbols;
    delete osymbols;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
