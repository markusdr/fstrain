#ifndef FSTRAIN_CREATE_V3_CREATE_TRIE_H
#define FSTRAIN_CREATE_V3_CREATE_TRIE_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/compose.h"
#include "fst/map.h"

//#include "fstrain/util/data.h"
//#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/normalize.h"
#include "fstrain/util/symbols-mapper-in-out-align.h"

#include "fstrain/create/ngram-counter.h"
#include "fstrain/create/v3-alignment-lattices-iterator.h"
#include "fstrain/create/v3-get-first-and-last.h"
#include "fstrain/create/get-well-formed.h"
#include "fstrain/create/features/latent-annotations-util.h"

#include <string>
#include <map>
#include <vector>
#include <cstddef>

#include "boost/shared_ptr.hpp"

namespace fstrain { namespace create { namespace v3 {

/**
 * @brief Returns the input dimension of alignment character (e.g. a|x
 * => a).
 */
std::string GetInputSym(const std::string align_sym) {
  static const std::string sep = "|"; // as in "a|x"
  std::size_t sep_pos = align_sym.find(sep);
  if (sep_pos == std::string::npos) {
    FSTR_CREATE_EXCEPTION("not an alignment character: " << align_sym);
  }
  return align_sym.substr(0, sep_pos);
}

// second in the pair is LogWeight
struct Comp2ndAsWeight {
  // want a first if it has greater count (i.e. smaller neglog weight)
  template<class T>
  bool operator()(const T& a,
                  const T& b) const {
    return a.second.Value() < b.second.Value();
  }
};

/**
 * @brief Uses the unigram counts from the counts_trie to decide what
 * alignment symbols are essential (for each input symbol, add enough
 * alignment symbols to cover 99% of the probability mass).
 */
template<class Arc>
void ExtractEssentialAlignSyms(const fst::Fst<Arc>& counts_trie,
                               const fst::SymbolTable& syms,
                               const double threshold,
                               fst::SymbolTable* pruned_syms) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;
  pruned_syms->AddSymbol("eps");

  typedef std::vector< std::pair<Label,Weight> > LabelWeightVec;
  typedef std::map<std::string,LabelWeightVec> StringToLabels;
  StringToLabels labels; // labels (and their counts) per input symbol

  StateId s = counts_trie.Start();

  for (fst::ArcIterator<fst::Fst<Arc> > ait(counts_trie, s);
       !ait.Done(); ait.Next()) {
    const Arc& arc = ait.Value();
    const Label label = arc.ilabel; // e.g. "a|x"
    const std::string align_sym = syms.Find(label);
    if (align_sym == "") {
      FSTR_CREATE_EXCEPTION("could not find label " << label
                            << " in symbol table");
    }
    std::string input_sym = GetInputSym(align_sym);  // e.g. "a|x" => "a"
    const Weight count = fst::Times(arc.weight,
                                    counts_trie.Final(arc.nextstate));
    labels[input_sym].push_back( std::make_pair(label, count) );
  }

  for (typename StringToLabels::iterator sit = labels.begin();
       sit != labels.end(); ++sit) {
    const std::string& input_sym = sit->first;
    LabelWeightVec& labels = sit->second;
    std::sort(labels.begin(), labels.end(), Comp2ndAsWeight());
    Weight sum = Weight::One();
    for (typename LabelWeightVec::const_iterator it = labels.begin();
         it != labels.end(); ++it) {
      sum = fst::Plus(sum, it->second);
    }
    Weight accumulated = Weight::One();
    Weight limit = Times(sum, Weight(-log(threshold)));
    double limit_val = limit.Value(); // e.g. -log(4 * 0.99)
    FSTR_CREATE_DBG_MSG(10, "Sym '"<<input_sym<<"', Limit: " << limit_val << " (-log("<<exp(-limit_val)<<"))"
                      << std::endl);
    for (typename LabelWeightVec::const_iterator it = labels.begin();
         it != labels.end(); ++it) {
      Label label = it->first; // e.g. label of "a|x"
      const std::string& sym = syms.Find(label); // "a|x"
      pruned_syms->AddSymbol(sym, label);
      FSTR_CREATE_DBG_MSG(10, "Added " << sym << std::endl);
      accumulated = fst::Plus(accumulated, it->second); // e.g. -log(2)=-0.69
      // std::cerr << "accumulated: " << accumulated << " (-log("<<exp(-accumulated.Value())<<"))" << std::endl;
      if (accumulated.Value() < limit_val) {
        break;
      }
    }
  }
}

/**
 * @brief Adds all identity symbols (a|a, b|b, ...) from align_sym to
 * the result (using the original labels).
 */
void AddIdentitySyms(const fst::SymbolTable& align_syms,
                     fst::SymbolTable* result) {
  static std::string sep = "|";
  fst::SymbolTableIterator it(align_syms);
  it.Next(); // ignore eps
  for (; !it.Done(); it.Next()) {
    const std::string align_sym = it.Symbol(); // e.g. a|x
    std::string input_sym = GetInputSym(it.Symbol());
    if (input_sym == "-") { // identity_sym "-|-" not allowed
      continue;
    }
    const std::string identity_sym = input_sym + sep + input_sym;
    if (align_sym == identity_sym) {
      // add with original label (it.Value())
      result->AddSymbol(align_sym, it.Value());
    }
  }
}

/**
 * @brief Creates the project-up and project-down FSTs and the
 * wellformed FSA.
 */
void GetProjAndWellformed(const fst::SymbolTable* isymbols,
                          const fst::SymbolTable* osymbols,
                          std::size_t max_insertions,
                          std::size_t num_conjugations,
                          std::size_t num_change_regions,
                          fst::MutableFst<fst::StdArc>* proj_up,
                          fst::MutableFst<fst::StdArc>* proj_down,
                          fst::MutableFst<fst::StdArc>* wellformed) {
  v3::GetFirstAndLast(isymbols, osymbols,
                      proj_up, proj_down,
                      num_conjugations, num_change_regions);

  getWellFormed(wellformed, proj_up->OutputSymbols(),
                max_insertions, num_conjugations, num_change_regions);
}

/**
 * @brief Creates an FST that maps symbols with no latent
 * annotations (nola, e.g. "a|x") to symbols with latent
 * annotations (e.g. "a|x-c2-g1").
 */
template<class Arc>
void CreateNolaToLaFst(const fst::SymbolTable* la_syms,
                       fst::SymbolTable* nola_syms,
                       fst::MutableFst<Arc>* result) {
  using namespace fst;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  StateId s = result->AddState();
  result->SetStart(s);
  result->SetFinal(s, Weight::One());
  result->SetInputSymbols(nola_syms);
  result->SetOutputSymbols(la_syms);
  nola_syms->AddSymbol("eps", 0);
  SymbolTableIterator it(*la_syms);
  it.Next(); // ignore eps
  for(; !it.Done(); it.Next()) {
    std::string la_sym = it.Symbol();
    int64 la_val = it.Value();
    // e.g. splits into "a|x" and "c2-g1"
    std::pair<std::string,std::string> split_sym =
        features::SplitMainFromLatentAnnotations(la_sym, '-');
    std::string nola_sym = split_sym.first;
    int64 nola_val = nola_syms->AddSymbol(nola_sym);
    assert(nola_val != -1);
    result->AddArc(s, Arc(nola_val, la_val, Weight::One(), s));
  }
}

/**
 * @brief Extracts ngram counts from the lattices that the iterator
 * provides; all lattices are converted to FSAs (e.g. tuple a:x is
 * changed to a|x character), and intersected with wellformed_fst
 * (which will impose max ins limit, add conj classes etc.).
 */
template<class Arc>
void AlignDataAndExtractNgramCounts(v3::AlignmentLatticesIterator<fst::StdArc> lattices_iter,
                                    std::size_t ngram_order,
                                    const fst::Fst<fst::StdArc>& wellformed_fst,
                                    const bool wellformed_has_latent_annotations,
                                    fst::MutableFst<Arc>* result_trie) {
  using namespace fst;

  const fst::SymbolTable* isymbols = lattices_iter.InputSymbols();
  const fst::SymbolTable* osymbols = lattices_iter.OutputSymbols();

  typedef WeightConvertMapper<StdArc, Arc> Map_SA;

  // should be LogArc or similar
  NgramCounter<Arc>* ngram_counter = new NgramCounter<Arc>(ngram_order);

  fst::SymbolTable* align_syms_nola = NULL; // nola = no latent ann.
  fst::MutableFst<StdArc>* proj_nola_to_la = NULL;
  if (wellformed_has_latent_annotations) {
    align_syms_nola = new fst::SymbolTable("align-syms-nola");
    proj_nola_to_la = new fst::VectorFst<StdArc>();
    // a|x => a|x-c1, a|x-c2, ...
    CreateNolaToLaFst(wellformed_fst.InputSymbols(),
                      align_syms_nola, proj_nola_to_la);
    proj_nola_to_la->SetInputSymbols(align_syms_nola);
  }
  else {
    align_syms_nola = wellformed_fst.InputSymbols()->Copy();
  }

  std::size_t cnt = 0;
  for (; !lattices_iter.Done(); lattices_iter.Next(), ++cnt) {
    std::cerr << "DATA " << cnt << std::endl;
    typedef typename v3::AlignmentLatticesIterator<StdArc>::FstPtr FstPtr;
    FstPtr aligned = lattices_iter.Value();
    Map(aligned.get(),
        util::SymbolsMapper_InOutToAlign<StdArc>(*isymbols, *osymbols,
                                                 *align_syms_nola));
    aligned->SetInputSymbols(align_syms_nola);
    aligned->SetOutputSymbols(align_syms_nola);

    VectorFst<StdArc> aligned2;
    if(wellformed_has_latent_annotations) {
      ComposeFst<StdArc> tmp(*aligned, *proj_nola_to_la);
      Compose(tmp,
              wellformed_fst,
              &aligned2);
      Project(&aligned2, PROJECT_OUTPUT);
    }
    else {
      Compose(*aligned, wellformed_fst, &aligned2);
    }
    aligned2.SetInputSymbols(NULL);
    aligned2.SetOutputSymbols(NULL);

    VectorFst<Arc> aligned3;
    Map(aligned2, &aligned3, Map_SA());
    util::Normalize(&aligned3);

    fst::Push(&aligned3, fst::REWEIGHT_TO_INITIAL);
    ngram_counter->AddCounts(aligned3);
  }
  delete align_syms_nola;
  delete proj_nola_to_la;
  ngram_counter->GetResult(result_trie);

  delete ngram_counter;
}

} } } // end namespaces

#endif
