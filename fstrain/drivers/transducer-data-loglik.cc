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
#include "fst/shortest-distance.h"
#include "fst/symbol-table.h"

#include "fstrain/util/approx-determinize.h"
#include "fstrain/util/data.h"
#include "fstrain/util/get-vector-fst.h"
#include "fstrain/util/print-path.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/drivers/debug.h"

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
      if (hypothesis == token) {
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
  DecodeDataOptions(const SymbolTable& isymbols_, const SymbolTable& osymbols_)
      : out(&std::cout), isymbols(isymbols_), osymbols(osymbols_)
  {}
};

LogArc::Weight GetPathsum(const Fst<LogArc>& fst) {
  std::vector<LogArc::Weight> betas;
  ShortestDistance(fst, &betas, true); // reverse
  if (betas.size() == 0) {
    return LogWeight::Zero();
  }
  return betas[0];
}

LogArc::Weight GetLoglik(const Fst<LogArc>& input_fst,
                         const Fst<LogArc>& output_fst,
                         const Fst<LogArc>& model_fst) {
  ComposeFst<LogArc> denominator(input_fst, model_fst);
  LogWeight denominator_sum = GetPathsum(denominator);
  if (denominator_sum == LogWeight::Zero()) {
    std::cerr << "WARNING: Even input received 0 prob" << std::endl;
  }
  VectorFst<LogArc> composed;
  Compose(denominator, output_fst, &composed);
  LogWeight numerator_sum = GetPathsum(composed);
  return Divide(numerator_sum, denominator_sum);
}

template<class EqualFct>
void DecodeData(const util::Data& data,
		const Fst<LogArc>& model_fst,
                EqualFct equal_fct,
		DecodeDataOptions opts) {
  const std::string separator = " ### "; // TODO: pass in
  int num_correct = 0;
  LogWeight corpus_loglik(LogWeight::One());
  int excluded_count = 0;
  for (util::Data::const_iterator it = data.begin(); it != data.end(); ++it) {
    const std::string input_string = it->first;
    const std::string output_string = it->second;
    LogWeight best_of_multiple(LogWeight::Zero());
    std::list<std::string> alternatives_list;
    boost::iter_split(alternatives_list, output_string,
                      boost::first_finder(separator));
    int cnt = 1;
    BOOST_FOREACH(std::string output_alternative, alternatives_list) {
      // std::cerr << cnt << ": " << input_string << " / " << output_alternative
      //   << std::endl;
      VectorFst<LogArc> input_fst;
      VectorFst<LogArc> output_fst;
      try{
        util::ConvertStringToFst(input_string, opts.isymbols, &input_fst);
        util::ConvertStringToFst(output_alternative, opts.isymbols, &output_fst);
      }
      catch(...) {
        std::cerr << "WARNING: Could not decode "
                  << input_string << " / " << output_alternative << std::endl;
      }
      LogWeight loglik = GetLoglik(input_fst, output_fst, model_fst);
      if (loglik.Value() < best_of_multiple.Value()) { // neg.loglik: smaller cost
        best_of_multiple = loglik;
      }
      ++cnt;
    }
    if (best_of_multiple != LogWeight::Zero()) {
      corpus_loglik = Times(corpus_loglik, best_of_multiple);
    }
    else {
      ++excluded_count;
    }
  }
  std::cout << corpus_loglik.Value() << " / " << data.size() << " = "
            << (corpus_loglik.Value() / data.size())
            << " Excluded: " << excluded_count << " examples with zero prob."
            << std::endl;
}

int main(int ac, char** av) {
  try{

    po::options_description generic("Allowed options");
    generic.add_options()
        ("config-file", po::value<std::string>(), "config file name")
        ("help", "produce help message")
        ("isymbols", po::value<std::string>(), "symbol table for input words")
        ("osymbols", po::value<std::string>(), "symbol table for output words")
        ("fst", po::value<std::string>(), "tranducer file name for decoding")
        ("multiple-truths", po::value<bool>()->default_value(true),
         "multiple truths, separated by ' ### '")
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

    if (vm.count("config-file")) {
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

    if (vm.count("input-file") == 0) {
      std::cerr << "Please give input file as argument" << std::endl;
      return EXIT_FAILURE;
    }

    if (vm.count("fst") == 0) {
      std::cerr << "Please alignment fst table with --fst" << std::endl;
      return EXIT_FAILURE;
    }

    SymbolTable* isymbols = SymbolTable::ReadText(vm["isymbols"].as<std::string>());
    SymbolTable* osymbols = SymbolTable::ReadText(vm["osymbols"].as<std::string>());

    const std::string data_filename = vm["input-file"].as<std::string>();
    const std::string fst_filename = vm["fst"].as<std::string>();

    util::Data data(data_filename);
    const Fst<LogArc>* fst = util::GetVectorFst<LogArc>(fst_filename);
    DecodeDataOptions opts(*isymbols, *osymbols);
    if (vm["multiple-truths"].as<bool>()) {
      const std::string separator = " ### ";
      DecodeData(data, *fst, MultipleAnswersCompare(separator), opts);
    }
    else {
      boost::is_equal equal_fct;
      DecodeData(data, *fst, boost::is_equal(), opts);
    }

    delete fst;
    delete isymbols;
    delete osymbols;

  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
