#ifndef FSTRAIN_CREATE_CREATE_SCORING_FSA_FROM_DATA_H
#define FSTRAIN_CREATE_CREATE_SCORING_FSA_FROM_DATA_H

#include "fst/fst.h"
#include "fst/map.h"
#include "fst/mutable-fst.h"
#include "fst/arcsort.h"
#include "fstrain/create/add-backoff.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/trie-insert-features.h"
#include "fstrain/create/v2/concat-start-and-end-labels.h"
#include "fstrain/create/v2/create-ngram-trie-from-data.h"
#include "fstrain/create/v2/get-project-up-down.h"
#include "fstrain/create/v2/get-pruned-alignment-syms.h"
// #include "fstrain/create/create-alignment-symbols.h" // for unpruned alphabet

#include "fstrain/create/create-alignment-symbols.h"

namespace fstrain { namespace create {

/**
 * @brief Creates scoring FSA: First prunes alignment alphabet, then
 * aligns data and extracts alignment ngrams.
 *
 * @param align_symbols The full alignment symbol table used in
 * proj_up and proj_down, or NULL. The pruned alphabet will be a
 * subset.
 */
template<class FeatArc, class Arc>
void CreateScoringFsaFromData(const util::Data& data,
                              const fst::SymbolTable& isyms,
                              const fst::SymbolTable& osyms,
                              const fst::Fst<Arc>& align_fst,
                              int ngram_order,
                              features::ExtractFeaturesFct& extract_features_fct,
                              const char sep_char,
                              fst::SymbolTable* pruned_align_syms,
                              bool add_identity_chars,
                              fst::SymbolTable* feature_names,
                              fst::MutableFst<FeatArc>* proj_up,
                              fst::MutableFst<FeatArc>* proj_down,
                              fst::MutableFst<FeatArc>* result) {
  using namespace fst;

  std::cerr << "# Getting pruned alignment symbols ..." << std::endl;
  GetPrunedAlignmentSyms(data, isyms, osyms, align_fst, pruned_align_syms);
  if(add_identity_chars){
    AddIdentityCharacters(isyms, osyms, pruned_align_syms);
  }
  std::cerr << "# Done getting pruned alignment symbols." << std::endl;
  // Unpruned:
  // CreateAlignmentSymbols(isyms, osyms, "", "", pruned_align_syms); // TEST

  int64 kStartLabel = pruned_align_syms->AddSymbol("START");
  int64 kEndLabel = pruned_align_syms->AddSymbol("END");
  int64 kPhiLabel = pruned_align_syms->AddSymbol("phi");
  std::cerr << "# Num of pruned alignment characters: " << pruned_align_syms->NumSymbols()
            << std::endl;

  fstrain::create::GetProjUpDown(isyms, osyms, *pruned_align_syms, sep_char,
                                 proj_up, proj_down);
  fstrain::create::ConcatStartAndEndLabels(kStartLabel, kEndLabel, true, false, proj_up);
  fstrain::create::ConcatStartAndEndLabels(kStartLabel, kEndLabel, false, true, proj_down);

  std::cerr << "# Creating ngram trie ..." << std::endl;
  CreateNgramTrieFromData(data,
                          isyms, osyms,
                          *proj_up, *proj_down,
                          *pruned_align_syms,
                          ngram_order,
                          result);
  std::cerr << "# Done creating ngram trie." << std::endl;
  // fstrain::util::printAcceptor(result, pruned_align_syms, std::cerr);

  fst::Map(result, fst::RmWeightMapper<FeatArc>());
  TrieInsertFeatures(*pruned_align_syms, feature_names, extract_features_fct, "",
                     result);
  std::cerr << "# Converting trie to ngram machine ..." << std::endl;
  ConvertTrieToModel(kStartLabel, kEndLabel, kPhiLabel, result);
  std::cerr << "# Done converting trie to ngram machine." << std::endl;

  ArcSort(proj_up, ILabelCompare<FeatArc>());
  ArcSort(proj_down, OLabelCompare<FeatArc>());
  ArcSort(result, ILabelCompare<FeatArc>());
}

} } // end namespace

#endif
