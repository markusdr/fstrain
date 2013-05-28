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
#ifndef FSTRAIN_CREATE_CONSTRUCT_LATTICE_FCT_H
#define FSTRAIN_CREATE_CONSTRUCT_LATTICE_FCT_H

#include "fst/arcsort.h"
#include "fst/compose.h"
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/project.h"
#include "fst/symbol-table.h"
#include "fst/invert.h"
#include "fst/rmepsilon.h"

#include "fstrain/create/debug.h"
#include "fstrain/create/get-first-and-last.h"
#include "fstrain/create/get-well-formed.h"
#include "fstrain/create/ngram-counter.h"
#include "fstrain/util/data.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/symbol-table-mapper.h"
//#include "fstrain/create/add-backoff.h"
//#include "fstrain/util/align-strings.h"
//#include "fstrain/util/options.h"
//#include "fstrain/util/remove-weights-keep-features-mapper.h"
//#include "fstrain/util/symbol-table-mapper.h"

#include <iostream>
#include <string>
#include <map>
#include <set>

#include "fstrain/util/symbols-mapper-in-out-align.h"
#include "fstrain/create/prune-fct.h"

namespace fstrain { namespace create {

struct ConstructLatticeFct {
  virtual ~ConstructLatticeFct() {};
  virtual void SetAlignmentSymbols(const fst::SymbolTable*) = 0;

  /**
   * @brief The final alignment syms can be different from the
   * alignment symbols you set with SetAlignmentSymbols: The symbols
   * may include latent annotations.
   */
  virtual const fst::SymbolTable* GetFinalAlignmentSymbols() const = 0;

  /**
   * @return Map from original align syms (no latent annotations,
   * e.g. "a|x") to final align syms which may contain latent
   * annotations (e.g. "a|x-c2-g1")
   */
  virtual const std::map<int64, std::set<int64> >& GetNolaToLaSymsMap() const = 0;

  /**
   * @brief Constructs lattice with all (valid) alignments for a given
   * input and output string.
   */
  virtual void operator()(const std::string& in, const std::string& out,
                          fst::MutableFst<fst::StdArc>* result) = 0;

  // TEST
  virtual void operator()(const std::string& in, const std::string& out,
                          const fst::SymbolTable& isymbols,
                          const fst::SymbolTable& osymbols,
                          const fst::Fst<fst::StdArc>& align_fst,
                          PruneFct* prune_fct,
                          fst::MutableFst<fst::StdArc>* result) = 0;
};

class ConstructLatticeFct_UpDown : public ConstructLatticeFct {

  /**
   * @brief Creates an FST that maps symbols with no latent
   * annotations ("nola", e.g. "a|x") to symbols with latent
   * annotations (e.g. "a|x-c2-g1").
   *
   * @see better version in v3-create-trie.h
   */
  template<class Arc>
  void CreateNolaToLaFst(const fst::SymbolTable* nola_syms,
                         const fst::SymbolTable* la_syms,
                         fst::MutableFst<Arc>* result) {
    using namespace fst;
    typedef typename Arc::Weight Weight;
    typedef typename Arc::StateId StateId;
    StateId s = result->AddState();
    result->SetStart(s);
    result->SetFinal(s, Weight::One());
    result->SetInputSymbols(nola_syms);
    result->SetOutputSymbols(la_syms);
    SymbolTableIterator it(*la_syms);
    it.Next(); // ignore eps
    for(; !it.Done(); it.Next()) {
      std::string la_sym = it.Symbol();
      int64 la_val = it.Value();
      // HACK: does not work for longer symbols, e.g. "the|DT-c2-g3" or "the|--c2-g3":
      std::string nola_sym = la_sym.substr(0, 3);
      int64 nola_val = nola_syms->Find(nola_sym);
      assert(nola_val != -1);
      result->AddArc(s, Arc(nola_val, la_val, Weight::One(), s));
    }
  }

  /**
   * @see CreateNolaToLaFst
   */
  void CreateNolaToLaMap(const fst::SymbolTable& nola_syms,
                         const fst::SymbolTable& la_syms,
                         std::map<int64, std::set<int64> >* result) {
    fst::SymbolTableIterator it(la_syms);
    it.Next(); // ignore eps
    for(; !it.Done(); it.Next()) {
      std::string la_sym = it.Symbol();
      int64 la_val = it.Value();
      // HACK: does not work for longer symbols, e.g. "the|DT-c2-g3" or "the|--c2-g3":
      std::string nola_sym = la_sym.substr(0, 3);
      int64 nola_val = nola_syms.Find(nola_sym);
      assert(nola_val != -1);
      (*result)[nola_val].insert(la_val);
    }
  }

 public:
  ConstructLatticeFct_UpDown(int max_insertions,
                             int num_conj, int num_regions,
                             bool symmetric)
      : proj_nola_to_la_(NULL),
        max_insertions_(max_insertions),
        num_conj_(num_conj), num_regions_(num_regions),
        symmetric_(symmetric)
  {
    proj_up_ = new fst::VectorFst<fst::StdArc>();
    proj_down_ = new fst::VectorFst<fst::StdArc>();
    wellformed_ = new fst::VectorFst<fst::StdArc>();
  }

  ~ConstructLatticeFct_UpDown() {
    delete proj_up_;
    delete proj_down_;
    delete wellformed_;
    delete proj_nola_to_la_;
  }

  fst::Fst<fst::StdArc>* GetWellformedFst() {
    return wellformed_;
  }

  fst::Fst<fst::StdArc>* GetProjUpFst() {
    return proj_up_;
  }

  fst::Fst<fst::StdArc>* GetProjDownFst() {
    return proj_down_;
  }

  /**
   * @brief Sets the alignment symbols that are to be used for
   * creating the lattices.
   *
   */
  void SetAlignmentSymbols(const fst::SymbolTable* syms) {
    align_syms_ = syms;
    std::set<char> ignore_set;
    getFirstAndLast(proj_up_, proj_down_,
                    ignore_set, ignore_set,
                    align_syms_,
                    num_conj_, num_regions_);
    getWellFormed(wellformed_, proj_up_->OutputSymbols(),
                  max_insertions_, num_conj_, num_regions_,
                  NULL, symmetric_);
    fst::ArcSort(wellformed_, fst::ILabelCompare<fst::StdArc>());
    assert(wellformed_->InputSymbols() != NULL);
    if(num_conj_ > 0 || num_regions_ > 0) {
      proj_nola_to_la_ = new fst::VectorFst<fst::StdArc>();
      CreateNolaToLaFst(align_syms_, wellformed_->InputSymbols(),
                        proj_nola_to_la_);
    }
    CreateNolaToLaMap(*align_syms_, *wellformed_->InputSymbols(),
                      &map_nola_to_la_);
  }

  virtual const fst::SymbolTable* GetFinalAlignmentSymbols() const {
    return proj_up_->OutputSymbols();
  }

  void operator()(const std::string& in, const std::string& out,
                  fst::MutableFst<fst::StdArc>* result) {
    using namespace fst;
    ComposeFstOptions<StdArc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    VectorFst<StdArc> in_fst;
    VectorFst<StdArc> out_fst;
    util::ConvertStringToFst(in, *proj_up_->InputSymbols(), &in_fst);
    util::ConvertStringToFst(out, *proj_down_->OutputSymbols(), &out_fst);
    in_fst.SetOutputSymbols(proj_up_->InputSymbols());
    out_fst.SetInputSymbols(proj_down_->OutputSymbols());
    ProjectFst<StdArc> in_proj(ComposeFst<StdArc>(in_fst, *proj_up_, copts),
                               PROJECT_OUTPUT);
    ProjectFst<StdArc> out_proj(ComposeFst<StdArc>(*proj_down_, out_fst, copts),
                                PROJECT_INPUT);
    ComposeFst<StdArc> in_and_wf(in_proj, *wellformed_, copts);
    Compose(in_and_wf, out_proj, result);
    result->SetInputSymbols(proj_up_->OutputSymbols());
    result->SetOutputSymbols(proj_up_->OutputSymbols());
  }

  void operator()(const std::string& in, const std::string& out,
                  const fst::SymbolTable& isymbols,
                  const fst::SymbolTable& osymbols,
                  const fst::Fst<fst::StdArc>& align_fst,
                  PruneFct* prune_fct,
                  fst::MutableFst<fst::StdArc>* result) {
    using namespace fst;
    ComposeFstOptions<StdArc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    VectorFst<StdArc> in_fst;
    VectorFst<StdArc> out_fst;
    util::ConvertStringToFst(in, isymbols, &in_fst);
    util::ConvertStringToFst(out, osymbols, &out_fst);
    VectorFst<StdArc> aligned;
    ComposeFst<StdArc> in_align(in_fst, align_fst, copts);
    Compose(in_align, out_fst, &aligned);
    if(prune_fct != NULL){
      (*prune_fct)(in, out, &aligned);
    }
    Map(&aligned,
        util::SymbolsMapper_InOutToAlign<StdArc>(isymbols, osymbols, *align_syms_));
    aligned.SetInputSymbols(align_syms_);
    aligned.SetOutputSymbols(align_syms_);

    if(proj_nola_to_la_ != NULL) {
      Compose(ComposeFst<StdArc>(aligned, *proj_nola_to_la_), *wellformed_, result);
      Project(result, PROJECT_OUTPUT);
    }
    else {
      Compose(aligned, *wellformed_, result);
    }
  }

  const std::map<int64, std::set<int64> >& GetNolaToLaSymsMap() const {
    return map_nola_to_la_;
  }

 private:
  const fst::SymbolTable* align_syms_;
  fst::MutableFst<fst::StdArc>* proj_up_;
  fst::MutableFst<fst::StdArc>* proj_down_;
  fst::MutableFst<fst::StdArc>* wellformed_;
  fst::MutableFst<fst::StdArc>* proj_nola_to_la_;
  std::map<int64, std::set<int64> > map_nola_to_la_;
  int max_insertions_;
  int num_conj_;
  int num_regions_;
  bool symmetric_;
};

}} // end namespaces

#endif
