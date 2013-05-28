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
#include "fst/map.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/train/online/train.h"
#include "fstrain/train/online/fst-stuff.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/data.h"
#include "fstrain/util/get-highest-feature-index.h"

// TEST
// #include "fstrain/util/print-fst.h"
#include "fstrain/train/set-feature-weights.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>
#include <boost/foreach.hpp>

namespace po = boost::program_options;

int main(int ac, char** av){
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("help", "produce help message")
        ("isymbols", po::value<std::string>(), "symbol table for input words")
        ("osymbols", po::value<std::string>(), "symbol table for output words")
        ("fst", po::value<std::string>(), "tranducer file name for decoding")
        ("train-data", po::value<std::string>(), "train data")
        ("dev-data", po::value<std::string>(), "dev data")
        ("output", po::value<std::string>(), "output")
        ("l1", po::value<bool>()->default_value(true), "cumulative L1")
        ("C", po::value<double>()->default_value(0.01), "cumulative L1 regularization strength")
        ("passes", po::value<int>()->default_value(20), "num of passes over the data")
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
      std::ifstream ifs(vm["config-file"].as<std::string>().c_str());
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

    if (vm.count("train-data") == 0) {
      std::cerr << "Please give train data with --train-data" << std::endl;
      return EXIT_FAILURE;
    }

    fst::SymbolTable* isymbols = fst::SymbolTable::ReadText(vm["isymbols"].as<std::string>());
    fst::SymbolTable* osymbols = fst::SymbolTable::ReadText(vm["osymbols"].as<std::string>());

    const std::string data_filename = vm["train-data"].as<std::string>();
    const std::string fst_filename = vm["fst"].as<std::string>();

    const bool l1 = vm["l1"].as<bool>();

    fstrain::util::Data data(data_filename);
    fst::MutableFst<fst::MDExpectationArc>* fst =
        fstrain::util::GetVectorFst<fst::MDExpectationArc>(fst_filename);

    int highest_feat_index = fstrain::util::getHighestFeatureIndex(*fst);
    std::vector<double> weights(highest_feat_index + 1);

    using namespace fstrain::train::online;

    // Batches* batches = new MockBatches();
    const int batch_size = 1;
    Batches* batches =
        new FstBatches<fst::MDExpectationArc>(*fst, &(weights[0]), *isymbols, *osymbols,
                                            data, batch_size);
    // LearningRateFct* rate_fct = new ConstantLearningRateFct(0.25);
    LearningRateFct* rate_fct = new ExponentialLearningRateFct(data.size());
    const int num_passes = vm["passes"].as<int>();
    const double C = vm["C"].as<double>();

//    if (vm.count("dev-data") == 0) {
//      std::cerr << "Please give dev data with --dev-data" << std::endl;
//      return EXIT_FAILURE;
//    }
//    const std::string dev_data_filename = vm["dev-data"].as<std::string>();
//    util::Data dev_data(dev_data_filename);

    if(l1) {
      CumulativeL1Trainer trainer(num_passes, *batches, *rate_fct, C, &weights);
      trainer.Train();
    }
    else {
      SgdTrainer trainer(num_passes, *batches, *rate_fct, &weights);
      trainer.Train();
    }

    if (vm.count("output") != 0) {
      std::string output = vm["output"].as<std::string>();
      std::string weights_file = output + ".feat-weights";
      std::ofstream out(weights_file.c_str());
      BOOST_FOREACH(double d, weights) {
        out << d << std::endl;
      }

      fstrain::train::SetFeatureWeights(&(weights[0]), fst);
      fst->Write(output + ".fst");

    }

    delete batches;
    delete rate_fct;

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
