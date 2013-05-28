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

#include <boost/program_options.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/create/ngram-counter.h"
#include "fstrain/util/print-fst.h"

namespace po = boost::program_options;

int main(int ac, char** av){
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("ngram-order", po::value<int>()->default_value(2), "ngram order")
        ("help", "produce help message")
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

    if (vm.count("help")) {
      std::cout << generic << "\n";
      return EXIT_FAILURE;
    }

    if(vm.count("input-file") == 0) {
      std::cerr << "Please pass observations as FST" << std::endl;
      exit(EXIT_FAILURE);
    }

    using namespace fstrain;
    // typedef fst::MDExpectationArc Arc;
    typedef fst::StdArc Arc;

    const std::string filename = vm["input-file"].as<std::string>();
    fst::MutableFst<Arc>* fst = fst::VectorFst<Arc>::Read(filename);

    const int ngram_order = vm["ngram-order"].as<int>();
    create::NgramCounter<Arc>* ngram_counter =
        new create::NgramCounter<Arc>(ngram_order);
    ngram_counter->AddCounts(*fst);

    fst::VectorFst<Arc> trie;
    ngram_counter->GetResult(&trie);
    util::printAcceptor(&trie, NULL, std::cout);

    delete ngram_counter;

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
