#ifndef FSTRAIN_V3_ALIGNMENT_LATTICES_ITERATOR_H
#define FSTRAIN_V3_ALIGNMENT_LATTICES_ITERATOR_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/compose.h"
#include "fst/matcher.h"

#include "fstrain/util/data.h"
#include "fstrain/util/options.h"
#include "fstrain/util/string-to-fst.h"

#include "fstrain/create/prune-fct.h"

#include "boost/shared_ptr.hpp"

namespace fstrain { namespace create { namespace v3 {

template<class A>
class AlignmentLatticesIterator {

 public:
  typedef A Arc;
  typedef typename boost::shared_ptr< fst::MutableFst<Arc> > FstPtr;

  AlignmentLatticesIterator(util::Data::const_iterator begin,
                            util::Data::const_iterator end,
                            const fst::Fst<fst::StdArc>& align_fst,
                            const fst::SymbolTable* isymbols,
                            const fst::SymbolTable* osymbols)
      : begin_(begin), end_(end), curr_(begin),
        align_fst_(align_fst), isymbols_(isymbols), osymbols_(osymbols),
        prune_fct_(NULL), use_sigma_label_(false)
  {
    if (util::options.has("sigma_label")) {
      sigma_label_ = util::options.get<int>("sigma_label");
      use_sigma_label_ = true;
    }
  }

  void SetPruneFct(PruneFct* prune_fct) {
    prune_fct_ = prune_fct;
  }

  bool Done() const {
    return curr_ == end_;
  }

  void Next() {
    ++curr_;
  }

  const fst::SymbolTable* InputSymbols() const {
    return isymbols_;
  }

  const fst::SymbolTable* OutputSymbols() const {
    return osymbols_;
  }

  FstPtr Value() const {
    using namespace fst;
    VectorFst<Arc> in_fst;
    VectorFst<Arc> out_fst;
    util::ConvertStringToFst(curr_->first, *isymbols_, &in_fst);
    util::ConvertStringToFst(curr_->second, *osymbols_, &out_fst);
    MutableFst<Arc>* aligned = new VectorFst<Arc>();

    if (use_sigma_label_) {
      typedef fst::Fst<Arc> FST;
      typedef fst::SigmaMatcher< fst::SortedMatcher<FST> > SM;
      fst::ComposeFstOptions<Arc, SM> copts1;
      copts1.gc_limit = 0;  // Cache only the last state for fastest copy.
      copts1.matcher1 = new SM(in_fst, MATCH_NONE, kNoLabel);
      copts1.matcher2 = new SM(
          align_fst_, MATCH_INPUT, sigma_label_, MATCHER_REWRITE_AUTO);
      ComposeFst<Arc> in_align(in_fst, align_fst_, copts1);
      if (in_align.Start() == kNoStateId) {
        FSTR_CREATE_EXCEPTION("composition of input and align fst is empty");
      }

      fst::ComposeFstOptions<Arc, SM> copts2;
      copts2.gc_limit = 0;  // Cache only the last state for fastest copy.
      copts2.matcher1 = new SM(
          in_align, MATCH_OUTPUT, sigma_label_, MATCHER_REWRITE_AUTO);
      copts2.matcher2 = new SM(out_fst, MATCH_NONE, kNoLabel);
      ComposeFst<Arc> composed(in_align, out_fst, copts2);
      if (composed.Start() == kNoStateId) {
        FSTR_CREATE_EXCEPTION("composition of aligned input and output is empty");
      }
      *aligned = composed; // copy
    }
    else {
      fst::ComposeFstOptions<Arc> copts;
      copts.gc_limit = 0;  // Cache only the last state for fastest copy.
      ComposeFst<Arc> in_align(in_fst, align_fst_, copts);
      Compose(in_align, out_fst, aligned);
    }

    if (prune_fct_ != NULL) {
      (*prune_fct_)(curr_->first, curr_->second, aligned);
    }
    return FstPtr(aligned);
  }

 private:
  util::Data::const_iterator begin_, end_, curr_;
  const fst::Fst<fst::StdArc>& align_fst_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  PruneFct* prune_fct_;
  bool use_sigma_label_;
  int64 sigma_label_;
};

} } } // end namespaces

#endif
