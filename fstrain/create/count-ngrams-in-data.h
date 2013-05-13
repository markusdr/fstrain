#ifndef FSTRAIN_CREATE_COUNT_NGRAMS_IN_DATA_H
#define FSTRAIN_CREATE_COUNT_NGRAMS_IN_DATA_H

#include "fst/arcsort.h"
// #include "fst/compose.h"
// #include "fst/project.h"
#include "fst/fst.h"
#include "fst/symbol-table.h"

#include "fstrain/create/ngram-counter.h"
//#include "fstrain/create/add-backoff.h"
#include "fstrain/create/debug.h"
#include "fstrain/util/align-strings.h"
#include "fstrain/util/data.h"
#include "fstrain/util/options.h"
#include "fstrain/util/print-fst.h"
//#include "fstrain/util/remove-weights-keep-features-mapper.h"
//#include "fstrain/util/symbol-table-mapper.h"
#include "fstrain/create/construct-lattice-fct.h"
#include "fstrain/create/get-alignment-symbols-fct.h"

#include <iostream>
#include <sstream>
#include <string>

namespace fstrain { namespace create {

/**
 * @brief First, determines pruned alignment alphabet by aligning the
 * data, then for each data pair, counts all alignments under the
 * pruned alignment alphabet.
 */
template<class Arc>
void CountNgramsInData(const util::Data& data,
                       const fst::SymbolTable& isymbols,
                       const fst::SymbolTable& osymbols,
                       const fst::Fst<fst::StdArc>& align_fst,
                       PruneFct* prune_fct,
                       GetAlignmentSymbolsFct* get_alignment_symbols_fct,
                       ConstructLatticeFct* construct_lattice_fct,
                       int ngram_order,
                       fst::SymbolTable*& ngram_trie_symbols,
                       fst::MutableFst<Arc>* ngram_trie){
  using namespace fst;
  typedef WeightConvertMapper<StdArc, Arc> Map_SL;
  std::stringstream aligned_data;
  SymbolTable align_symbols("align-symbols");
  util::AlignStringsDefaultOutputStream<std::stringstream> out(&aligned_data, &align_symbols);
  util::AlignStringsOptions opts;
  opts.n_best_alignments = 1; // one-best
  if(fstrain::util::options.has("sigma_label")){
    opts.sigma_label = fstrain::util::options.get<int>("sigma_label");
  }
  util::AlignStrings(data, isymbols, osymbols, align_fst, &out, opts);  
  SymbolTable pruned_syms("pruned-syms");
  (*get_alignment_symbols_fct)(aligned_data, isymbols, osymbols, &pruned_syms);
  std::cerr << "# Extracted " << pruned_syms.NumSymbols() << " alignment symbols." << std::endl;
  FSTR_CREATE_DBG_EXEC(10,
                       pruned_syms.WriteText(std::cerr);
                       std::cerr << std::endl;
                       );
  construct_lattice_fct->SetAlignmentSymbols(&pruned_syms);
  ngram_trie_symbols = construct_lattice_fct->GetFinalAlignmentSymbols()->Copy();

  NgramCounter<Arc>* ngram_counter = new NgramCounter<Arc>(ngram_order);
  
  for(util::Data::const_iterator d = data.begin(); d != data.end(); ++d) {
    // std::cerr << d->first << " -- " << d->second << std::endl;
    VectorFst<StdArc> lattice;
    if(prune_fct == NULL) {
      // old version uses proj_up and proj_down:
      (*construct_lattice_fct)(d->first, d->second, &lattice);
    }
    else {
      // could always use this as well:
      (*construct_lattice_fct)(d->first, d->second, isymbols, osymbols, 
                               align_fst, prune_fct, 
                               &lattice);
    }
    FSTR_CREATE_DBG_EXEC(10,
                         std::cerr << "ACCEPTOR:" << std::endl;
                         util::printAcceptor(&lattice, lattice.InputSymbols(), std::cerr);
                         std::cerr << std::endl;);
    lattice.SetInputSymbols(NULL);
    lattice.SetOutputSymbols(NULL);
    MapFst<StdArc, Arc, Map_SL> mapped(lattice, Map_SL());
    ngram_counter->AddCounts(mapped);
  }

  ngram_counter->GetResult(ngram_trie);
  delete ngram_counter;
}

} } // end namespaces


#endif
