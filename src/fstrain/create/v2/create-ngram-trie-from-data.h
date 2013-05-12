#ifndef FSTRAIN_CREATE_V2_CREATE_NGRAM_TRIE_FROM_DATA_H
#define FSTRAIN_CREATE_V2_CREATE_NGRAM_TRIE_FROM_DATA_H

#include <cstdlib> // size_t
#include "fst/fst.h"
#include "fst/vector-fst.h"
#include "fst/symbol-table.h"
#include "fst/mutable-fst.h"
#include "fstrain/util/data.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/create/ngram-counter.h"
#include "fstrain/create/v2/create-lattice.h"
#include "fstrain/create/v2/get-sigma-fsa.h"

namespace fstrain { namespace create {

/**
 * @brief Creates ngram trie by looking at all alignments of the data,
 * extracting the alignment ngrams that occur...
 */
template<class Arc>
void CreateNgramTrieFromData(
    const util::Data& data,
    const fst::SymbolTable& isyms,
    const fst::SymbolTable& osyms,
    const fst::Fst<Arc>& proj_up,
    const fst::Fst<Arc>& proj_down,
    const fst::SymbolTable& align_syms,
    size_t ngram_order,
    fst::MutableFst<Arc>* result) {
  using namespace fst;
  using util::Data;
  NgramCounter<Arc>* ngram_counter = new NgramCounter<Arc>(ngram_order);
  int cnt = 0;
  for(Data::const_iterator it = data.begin(); it != data.end(); ++it, ++cnt){
    if(cnt % 1000 == 0){
      std::cerr << cnt << std::endl;
    }
    VectorFst<Arc> input;
    VectorFst<Arc> output;
    util::ConvertStringToFst(it->first, isyms, &input);
    util::ConvertStringToFst(it->second, osyms, &output);
    VectorFst<Arc> lattice;
    CreateLattice(input, output, proj_up, proj_down, &lattice);
    ngram_counter->AddCounts(lattice);
  }

  VectorFst<Arc> unigrams; // also observe any unigram
  GetSigmaFsa(align_syms, &unigrams);
  ngram_counter->AddCounts(unigrams);

  ngram_counter->GetResult(result);
  delete ngram_counter;
}

} } // end namespace

#endif
