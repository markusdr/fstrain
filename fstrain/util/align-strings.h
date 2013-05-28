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
#ifndef UTIL_ALIGN_STRINGS_H
#define UTIL_ALIGN_STRINGS_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstddef>
#include "fstrain/util/data.h"
#include "fstrain/util/debug.h"
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/shortest-path.h"
#include "fst/arcsort.h"
#include "fst/compose.h"
#include "fst/matcher.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/mutable-fst.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/print-path.h"

namespace fstrain { namespace util {

struct AlignStringsOptions {
  std::string separator;
  size_t n_best_alignments;
  int sigma_label;
  AlignStringsOptions()
      : separator("|"), n_best_alignments(1),
        sigma_label(fst::kNoLabel) {}
};

template<class OutputStream>
struct AlignStringsDefaultOutputStream {
  OutputStream* out_;
  fst::SymbolTable* syms_;
  AlignStringsDefaultOutputStream(OutputStream* out,
                                  fst::SymbolTable* syms)
      : out_(out), syms_(syms) {
    syms->AddSymbol("eps", 0);
  }
  // for std::endl:
  OutputStream& operator<<(OutputStream& (*f)(OutputStream&)) {
    return f(*out_);
  }
  OutputStream& operator<<(const std::string& str) {
    (*out_) << str;
    if (str != "" && str != " " && str != "\n") {
      syms_->AddSymbol(str);
    }
    return *out_;
  }
};


template<class Arc, class OutputStream>
void AlignStrings(const std::string& data_filename,
                  const fst::SymbolTable& isymbols,
                  const fst::SymbolTable& osymbols,
                  const fst::Fst<Arc>& fst,
                  OutputStream* out,
                  const AlignStringsOptions& opts)
{
  util::Data data(data_filename);
  AlignStrings(data, isymbols, osymbols, fst, out, opts);
}

template<class Arc, class OutputStream>
void AlignStrings(const util::Data& data,
                  const fst::SymbolTable& isymbols,
                  const fst::SymbolTable& osymbols,
                  const fst::Fst<Arc>& fst,
                  OutputStream* out,
                  const AlignStringsOptions& opts)
{
  using namespace fst;
  PrintTransducerArcFct<StdArc, OutputStream> print_arc_fct(&isymbols, &osymbols, out);
  print_arc_fct.separator = opts.separator;
  ArcSortFst<Arc, ILabelCompare<Arc> > sorted_fst(fst, ILabelCompare<Arc>());

  for (int i = 0; i < data.size(); ++i) {
    const std::pair<std::string, std::string>& d = data[i];
    MutableFst<Arc>* in_fst = new VectorFst<Arc>();
    util::ConvertStringToFst(d.first, isymbols, in_fst);
    MutableFst<Arc>* out_fst = new VectorFst<Arc>();
    util::ConvertStringToFst(d.second, osymbols, out_fst);

    typedef SigmaMatcher<Matcher< Fst<Arc> > > SM;

    ComposeFstOptions<Arc, SM> copts1;
    copts1.gc_limit = 0;
    copts1.matcher1 = new SM(*in_fst, MATCH_NONE);
    copts1.matcher2 = new SM(sorted_fst, MATCH_INPUT, opts.sigma_label);
    ComposeFst<Arc> composed1(*in_fst, sorted_fst, copts1);
    ArcSortFst<Arc, OLabelCompare<Arc> > sorted1(composed1, OLabelCompare<Arc>());

    ComposeFstOptions<Arc, SM> copts2;
    copts2.gc_limit = 0;
    copts2.matcher1 = new SM(sorted1, MATCH_OUTPUT, opts.sigma_label);
    copts2.matcher2 = new SM(*out_fst, MATCH_NONE);
    ComposeFst<Arc> composed2(sorted1, *out_fst, copts2);

    typedef WeightConvertMapper<Arc, StdArc> Map_AS;
    MapFst<Arc, StdArc, Map_AS> mapped(composed2, Map_AS());
    VectorFst<StdArc> best_path;
    ShortestPath(mapped, &best_path, opts.n_best_alignments);
    delete in_fst;
    delete out_fst;
    if (best_path.Start() == fst::kNoStateId || best_path.NumStates() == 0) {
      std::cerr << "Cannot align example: " << d.first << " / " << d.second
                << std::endl;
      continue;
    }
    if (opts.n_best_alignments == 1) {
      PrintPath(best_path, print_arc_fct);
      // (*out) << std::endl;     // TODO: why doesn't this compile?
      (*out) << "" << std::endl;  // workaround
    }
    else {
      FSTR_UTIL_EXCEPTION(opts.n_best_alignments << "-best paths unimplemented");
    }
  }
}

} } // end namespaces

#endif
