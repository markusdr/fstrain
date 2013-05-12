#ifndef FSTRAIN_CREATE_NOEPSHIST_CREATE_SCORING_FST_H
#define FSTRAIN_CREATE_NOEPSHIST_CREATE_SCORING_FST_H

#include "fst/symbol-table.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/concat.h"
#include <string>
#include "fstrain/create/debug.h"
#include "fstrain/create/feature-functions.h"
#include "fstrain/util/symbol-table-mapper.h"
#include "fstrain/util/misc.h" // HasOutArcs
#include "fstrain/util/print-fst.h"
#include "fstrain/util/data.h"
#include <queue>
#include <set>

#include "fstrain/create/noepshist/backoff-iterator.h"
#include "fstrain/create/noepshist/history-filter.h"
#include "fstrain/create/create-scoring-fst-from-trie.h"
#include "fstrain/util/options.h"
#include "fstrain/create/get-alignment-symbols-fct.h"
#include "fstrain/create/construct-lattice-fct.h"
#include "fstrain/create/count-ngrams-in-data.h"

namespace fstrain { namespace create { namespace noepshist {

void CheckHistory(const std::string& hist, const char sep_char) {
  typedef std::string::size_type stype;
  stype sep_pos = hist.find(sep_char);    
  if(sep_pos == std::string::npos){
    FSTR_CREATE_EXCEPTION("history '"<< hist <<"' has no separator '" 
                          << sep_char << "'");
  }
}

std::string GetBackoffHistory(const std::string& hist, 
                              HistoryFilter* hist_filter,
                              const char sep_char) {
  if(hist_filter == NULL){
    return hist;
  }
  for(BackoffIterator biter(hist, sep_char); !biter.Done(); biter.Next()){
    const std::string backoff = biter.Value();
    if((*hist_filter)(backoff)){
      return backoff;
    }
  }
  FSTR_CREATE_EXCEPTION("Could not find any allowed backoff for '" << 
                        hist << "'");
}

/**
 * @brief Extends history by one symbol.
 *
 * @param hist The original history, e.g. "ab|xy". 
 * @param sym The symbol by which to extend, e.g. "c|z"
 */
std::string ExtendHistory(const std::string& hist, const std::string& sym, 
                          const char sep_char, const char eps_char) {
  std::stringstream result;
  CheckHistory(hist, sep_char);
  std::string lhist, rhist;
  std::stringstream ss(hist);
  std::getline(ss, lhist, sep_char); // tokenizer
  std::getline(ss, rhist, sep_char);
  const char lchar = sym[0]; // a in a|x
  const char rchar = sym[2]; // x in a|x
  result << lhist; 
  if(lchar != eps_char){
    result << lchar;
  }
  result << sep_char << rhist;
  if(rchar != eps_char){
    result << rchar;
  }
  return result.str();
}

template<class Arc>
void GetStateHistories(const fst::Fst<Arc>& fst, 
                       HistoryFilter* hist_filter,
                       const std::string& start_hist0,
                       const char sep_char,
                       const char eps_char,
                       const fst::SymbolTable& align_syms,
                       const std::string& end_align_sym,
                       bool all_final_states_have_same_history,
                       bool store_leaves_histories,
                       std::set<std::string>* histories) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  std::queue<StateId> states_todo;
  StateId start_id = fst.Start();
  states_todo.push(start_id);
  const std::string start_hist = GetBackoffHistory(start_hist0, hist_filter, 
                                                   sep_char);
  // histories->AddSymbol(start_hist, start_id);
  histories->insert(start_hist);
  typedef std::map<StateId, std::string> Id2HistMap;
  typedef typename Id2HistMap::const_iterator Id2HistMapIter;
  Id2HistMap id2hist;
  id2hist.insert(std::make_pair(start_id, start_hist));
  while(!states_todo.empty()){
    StateId state_id = states_todo.front();
    states_todo.pop();
    // const std::string state_hist = histories->Find(state_id);
    Id2HistMapIter found = id2hist.find(state_id);
    if(found == id2hist.end()) {
      FSTR_CREATE_EXCEPTION("Could not find history of state " << state_id);
    }
    const std::string state_hist = found->second;
    FSTR_CREATE_DBG_MSG(10, "hist("<<state_id<<")=" << state_hist << std::endl);
    ArcIterator<Fst<Arc> > aiter(fst, state_id);
    for(; !aiter.Done(); aiter.Next() ){
      const Arc& arc = aiter.Value();
      const std::string sym = align_syms.Find(arc.ilabel);
      bool next_state_is_leaf = !util::HasOutArcs(fst, arc.nextstate);
      if(next_state_is_leaf && !store_leaves_histories){
        continue;
      }
      const std::string arc_hist = ExtendHistory(state_hist, sym, 
                                                 sep_char, eps_char);
      FSTR_CREATE_DBG_MSG(10, "arc_hist=" << arc_hist << std::endl);
      std::string next_state_hist = GetBackoffHistory(arc_hist, hist_filter, 
                                                      sep_char);
      if(next_state_is_leaf && all_final_states_have_same_history){
        next_state_hist = end_align_sym;
      }
      //std::string next_state_hist_lookup = histories->Find(arc.nextstate);
      //if(next_state_hist_lookup == ""){ // not contained
      Id2HistMapIter found = id2hist.find(arc.nextstate);
      if(found == id2hist.end()){
        std::cerr << "Adding hist " << next_state_hist 
                  << " (id=" << arc.nextstate << ")" << std::endl;
        // histories->AddSymbol(next_state_hist, arc.nextstate);
        histories->insert(next_state_hist);
        id2hist.insert(std::make_pair(arc.nextstate, next_state_hist));
        states_todo.push(arc.nextstate);
      }
      else if(next_state_hist != found->second) {
        FSTR_CREATE_EXCEPTION("histories for state " << arc.nextstate 
                              << " do not agree: " 
                              << next_state_hist << " != " << found->second);
      }
    }
  }  
}

/**
 * @brief Interface for function class that generates and inserts
 * features.
 */
template<class Container>
class InsertFeaturesFct {
 public:
  virtual ~InsertFeaturesFct() {}
  virtual void operator()(const std::string& state_history, 
                          const std::string& arcsym,
                          Container* container,  
                          fst::SymbolTable* feature_ids) = 0;
  virtual void SetFilter(HistoryFilter*) = 0;
};

/**
 * @brief Function class that generates and inserts basic features.
 */
template<class Container>
class InsertFeaturesFct_Default : public InsertFeaturesFct<Container> {
  const char sep_char_;
  const char eps_char_;
  virtual ~InsertFeaturesFct_Default() {}
  HistoryFilter* hist_filter_;

 public:

  explicit InsertFeaturesFct_Default(const char sep_char, const char eps_char) 
      : sep_char_(sep_char), eps_char_(eps_char), hist_filter_(NULL) 
  {}

  void operator()(const std::string& state_history, 
                  const std::string& arcsym,
                  Container* container, 
                  fst::SymbolTable* feature_ids) {
    if(feature_ids == NULL){
      FSTR_CREATE_EXCEPTION("feature_ids is NULL");
    }
    typedef std::set<std::string> StringSet;
    StringSet new_feats;
    for(BackoffIterator biter(state_history, sep_char_); !biter.Done(); biter.Next()){
      const std::string& backoff = biter.Value();
      const std::string new_feat = 
          ExtendHistory(backoff, arcsym, sep_char_, eps_char_);
      if(hist_filter_ == NULL || (*hist_filter_)(backoff)){        
        new_feats.insert(new_feat);
      }
    }
    if(fstrain::util::options.has("fstrain.create.lengthPenaltyFeatures")){
      AddLengthPenalties(arcsym, sep_char_, eps_char_, &new_feats);
    }
    if(fstrain::util::options.has("fstrain.create.insDelFeatures")){
      FSTR_CREATE_EXCEPTION("unimplemented");
      // AddInsDelFeatures(arcsym, sep_char_, eps_char_, &new_feats);
    }
    for(StringSet::const_iterator it = new_feats.begin(); it != new_feats.end(); ++it) {
      const int64 feature_id = feature_ids->AddSymbol(*it);
      container->insert(feature_id, 0.0);
    }
  }
  
  void SetFilter(HistoryFilter* filter) {
    hist_filter_ = filter;
  }

}; // end class


template<class Arc>
class ScoringFstBuilder {

 private:

  typedef typename Arc::Weight::MDExpectations FeaturesContainer;

  const char eps_char_;
  const char start_char_;
  const char sep_char_;
  InsertFeaturesFct<FeaturesContainer>* insert_features_fct_;

 public:

  ScoringFstBuilder(InsertFeaturesFct<FeaturesContainer>* iff = 
                    new InsertFeaturesFct_Default<FeaturesContainer>('|', '-')
                    ) 
      : eps_char_('-'), start_char_('S'), sep_char_('|'), 
        insert_features_fct_(iff)
  {}

  ScoringFstBuilder(
      const char eps_char, const char start_char, const char sep_char,
      InsertFeaturesFct<FeaturesContainer>* iff) 
      : eps_char_(eps_char), start_char_(start_char), sep_char_(sep_char),
        insert_features_fct_(iff)
  {}

  ~ScoringFstBuilder() {
    delete insert_features_fct_;
  }

  void SetFeaturesFilter(HistoryFilter* filter) {
    insert_features_fct_->SetFilter(filter);
  }
    
  void AddArc(typename Arc::StateId state_id_from,
              typename Arc::StateId state_id_to,
              typename Arc::Label label,
              const std::string& state_history,
              const std::string& arcsym,
              fst::MutableFst<Arc>* result,
              fst::SymbolTable* feature_ids) 
  {
    typedef typename Arc::Weight Weight;
    Weight w(1e-32);
    (*insert_features_fct_)(state_history, arcsym,
                            &(w.GetMDExpectations()), feature_ids);
    result->AddArc(state_id_from, 
                   Arc(label, label, w, state_id_to));
  }

  /**
   * @brief Adds arcs to a state, finds out the target states and adds
   * them to the queue.
   */
  void AddArcs(const fst::SymbolTable& align_syms,
               const std::string& end_align_sym,
               const typename Arc::StateId state_id, 
               fst::SymbolTable* state_ids,
               std::queue<typename Arc::StateId>* states_todo,
               HistoryFilter* hist_filter,
               fst::SymbolTable* feature_ids,
               fst::MutableFst<Arc>* result) {
    FSTR_CREATE_DBG_MSG(10, "State " << state_id << ": AddArcs" << std::endl);
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Weight Weight;
    std::string state_hist = state_ids->Find(state_id);
    if(state_hist.length() == 0){
      FSTR_CREATE_EXCEPTION("no history for state " << state_id);
    }
    fst::SymbolTableIterator aiter(align_syms);
    aiter.Next(); // ignore eps sym
    for( ; !aiter.Done(); aiter.Next()) {
      const std::string sym = aiter.Symbol();
      bool target_state_is_final = sym == end_align_sym;
      std::string target_state_hist = ExtendHistory(state_hist, sym, 
                                                    sep_char_, eps_char_);
      const std::string full_history = target_state_hist;
      if(target_state_is_final){
        target_state_hist = end_align_sym;
      }
      else {
        target_state_hist = 
            GetBackoffHistory(target_state_hist, hist_filter, sep_char_);
      }
      StateId target_state_id = state_ids->Find(target_state_hist);
      if(target_state_id == fst::kNoStateId){
        target_state_id = result->AddState();
        state_ids->AddSymbol(target_state_hist, target_state_id);
        if(!target_state_is_final){
          states_todo->push(target_state_id);
        }
      }
      if(target_state_is_final){
        result->SetFinal(target_state_id, Weight::One());              
      }
      FSTR_CREATE_DBG_MSG(10, "Found history " << target_state_hist 
                          << "(id="<<target_state_id<<")" << std::endl);
      AddArc(state_id, target_state_id, aiter.Value(), state_hist, sym,
             result, feature_ids);
      FSTR_CREATE_DBG_MSG(10, "Arc " << state_id << "[" << state_hist << "]\t"  
                          << target_state_id << "[" << target_state_hist << "]\t"
                          << sym << std::endl);
    }
  }

  /**
   * @brief Creates a scoring FST.
   * 
   * @param align_syms The alignment symbols.
   * @param end_align_sym The end symbol (e.g. "E|E").
   * @param hist_filter Filter that says what histories are allowed (NULL if all histories allowed)
   * @param feature_ids The resulting feature IDs.
   * @param result The resulting FST.
   */
  void CreateScoringFst(
      const fst::SymbolTable& align_syms,
      const std::string& end_align_sym,
      HistoryFilter* hist_filter,
      fst::SymbolTable* feature_ids,
      fst::MutableFst<Arc>* result) 
  {
    using namespace fst;
    typedef typename Arc::StateId StateId;
    std::queue<StateId> states_todo;
    SymbolTable state_ids("state_ids");
    std::stringstream start_hist_ss;
    start_hist_ss << start_char_ << sep_char_ << start_char_;
    std::string start_hist = GetBackoffHistory(start_hist_ss.str(), 
                                               hist_filter, sep_char_);
    StateId start_state_id = result->AddState();    
    state_ids.AddSymbol(start_hist, start_state_id);
    states_todo.push(start_state_id);
    result->SetStart(start_state_id);
    while(!states_todo.empty()) {
      StateId sid = states_todo.front();
      states_todo.pop();
      AddArcs(align_syms, end_align_sym, 
              sid, &state_ids, &states_todo,
              hist_filter,
              feature_ids, result);
    }

    FSTR_CREATE_DBG_EXEC(10,
                         if(hist_filter != NULL){
                           std::cerr << "Running GetStateHistories" << std::endl;
                           // SymbolTable histories("histories");
                           std::set<std::string> histories;
                           std::stringstream start_hist_ss;
                           start_hist_ss << start_char_ << sep_char_ << start_char_;
                           GetStateHistories(*result, hist_filter, start_hist_ss.str(), 
                                             sep_char_, eps_char_, 
                                             align_syms, end_align_sym, true, true, &histories);
                           std::cerr << "NEW:" << std::endl;
                           //for(SymbolTableIterator siter(histories); !siter.Done(); siter.Next()){
                           // std::cerr << siter.Value() << "\t" << siter.Symbol() << std::endl;
                           //}
                           for(std::set<std::string>::const_iterator it = histories.begin();
                               it != histories.end(); ++it){
                             std::cerr << *it << std::endl;
                           }
                           std::cerr << "ORIG:" << std::endl;
                           for(fst::SymbolTableIterator siter(state_ids); !siter.Done(); siter.Next()){
                             std::cerr << siter.Value() << "\t" << siter.Symbol() << std::endl;
                           }
                         }
                         );

  }

}; // end class

///

void CreateNgramFstFromBestAlign(size_t ngram_order,
				 int max_insertions,
				 int num_conjugations,
				 int num_change_regions,
				 const fst::Fst<fst::StdArc>& alignment_fst,
				 const std::string& data_filename,
				 const fst::SymbolTable* isymbols,
				 const fst::SymbolTable* osymbols,
				 double variance,
                                 fst::SymbolTable* feature_names,
				 fst::MutableFst<fst::MDExpectationArc>* result)  
{
  using namespace fst;
  util::Data data(data_filename);

  const char sep_char = '|';
  
  // TODO: actually use the flexibility of the new machine to allow
  // for the inout (or output) histories to be longer.
  HistoryFilter_Length hist_filter(ngram_order - 1, ngram_order - 1, sep_char);

  GetAlignmentSymbolsFct_AddIdentityChars get_align_syms_fct;
  bool symmetric = false;
  ConstructLatticeFct_UpDown construct_lattice_fct(max_insertions, 
                                                   num_conjugations, 
                                                   num_change_regions, 
                                                   symmetric);

  Fst<StdArc>* proj_up = construct_lattice_fct.GetProjUpFst();
  Fst<StdArc>* proj_down = construct_lattice_fct.GetProjDownFst();
  Fst<StdArc>* wellformed_fst = construct_lattice_fct.GetWellformedFst();

  SymbolTable* align_syms = NULL;
  CountNgramsInData(data, *isymbols, *osymbols, alignment_fst, NULL,
		    &get_align_syms_fct, &construct_lattice_fct,
		    // ngram_order,
		    ngram_order + 2,
		    align_syms,
		    result);
  const std::string end_sym = "E|E";
  // SymbolTable state_histories("state-histories");
  std::set<std::string> state_histories;
  fstrain::util::printAcceptor(result, align_syms, std::cerr);  
  std::stringstream start_hist_ss;
  start_hist_ss << sep_char; // empty history
  GetStateHistories(*result, &hist_filter, start_hist_ss.str(), sep_char, '-', 
                    *align_syms, end_sym, false, false, &state_histories);

  std::cerr << "FOUND HISTORIES:" << std::endl;
  for(std::set<std::string>::const_iterator it = state_histories.begin(); 
      it != state_histories.end(); ++it){
    std::cerr << *it << std::endl;
  }

  // Adds all alignment syms as allowed (unigram) history
  for(SymbolTableIterator it(*align_syms); !it.Done(); it.Next()){
    std::string unigram_hist = it.Symbol();
    if(unigram_hist[0] == '-'){ // -|x
      unigram_hist = unigram_hist.substr(1);
    }
    if(unigram_hist[2] == '-'){ // a|-
      unigram_hist = unigram_hist.substr(0, unigram_hist.length() - 1);
    }
    std::cerr << "Adding unigram hist " << unigram_hist << std::endl;
    state_histories.insert(unigram_hist);
  }
  ObservedHistoriesFilter observed_hists_filter(state_histories);

  ScoringFstBuilder<MDExpectationArc> sfb;
  //int maxdiff = 1; // e.g. allow "ab|xy" and "ab|y"
  //HistoryFilter_LengthDiff feat_filter(1, sep_char);
  //sfb.SetFeaturesFilter(&feat_filter);

  sfb.CreateScoringFst(*align_syms, end_sym, &observed_hists_filter, 
                       feature_names, result);

  // add start arc at beginning
  VectorFst<MDExpectationArc> start_fst;
  start_fst.AddState();
  start_fst.AddState();
  start_fst.SetStart(0);
  const int64 kStartLabel = align_syms->Find("S|S");
  start_fst.AddArc(0, 
                   MDExpectationArc(kStartLabel, kStartLabel, MDExpectationArc::Weight::One(), 1));
  start_fst.SetFinal(1, MDExpectationArc::Weight::One());
  Concat(start_fst, result);

  // compose with wellformed, proj_up, proj_down
  typedef WeightConvertMapper<StdArc, MDExpectationArc> Map_SE;
  MapFst<StdArc, MDExpectationArc, Map_SE> mapped(*wellformed_fst, Map_SE());
  typedef fstrain::util::RemoveSymbolTableMapper<MDExpectationArc> RmSymTableMapper;
  MapFst<MDExpectationArc, MDExpectationArc, RmSymTableMapper> mapped2(mapped, RmSymTableMapper());
  
  ArcSort(result, OLabelCompare<MDExpectationArc>());

  typedef util::SymbolTableMapper<StdArc> SymMapper;

  SymMapper sym_mapper1(*proj_up->InputSymbols(), *isymbols,
                        *proj_up->OutputSymbols(), *align_syms);
  MapFst<StdArc, StdArc, SymMapper> proj_up_mapped1(*proj_up, sym_mapper1);
  MapFst<StdArc, MDExpectationArc, Map_SE> proj_up_mapped2(proj_up_mapped1, Map_SE());
  
  SymMapper sym_mapper2(*proj_down->InputSymbols(), *align_syms,
                        *proj_down->OutputSymbols(), *osymbols);
  MapFst<StdArc, StdArc, SymMapper> proj_down_mapped1(*proj_down, sym_mapper2);
  MapFst<StdArc, MDExpectationArc, Map_SE> proj_down_mapped2(proj_down_mapped1, Map_SE());

  typedef ComposeFst<MDExpectationArc> CFst;
  ComposeFstOptions<MDExpectationArc> copts;
  copts.gc_limit = 0;
  *result = CFst(CFst(proj_up_mapped2, 
                      CFst(*result, mapped2, copts), copts),
                 proj_down_mapped2, copts);

} // end function


} } } // end namespaces

#endif
