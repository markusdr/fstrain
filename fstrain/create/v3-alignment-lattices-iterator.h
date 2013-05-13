#ifndef V3_ALIGNMENT_LATTICES_ITERATOR_H
#define V3_ALIGNMENT_LATTICES_ITERATOR_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/compose.h"

#include "fstrain/util/data.h"
#include "fstrain/util/string-to-fst.h"

#include "fstrain/create/prune-fct.h"

#include "boost/shared_ptr.hpp"

namespace fstrain { namespace create { namespace v3 {

template<class A>
class AlignmentLatticesIterator {

 public:
  typedef A Arc;
  
 private:
  util::Data::const_iterator begin_, end_, curr_;
  const fst::Fst<fst::StdArc>& align_fst_;
  const fst::SymbolTable* isymbols_;
  const fst::SymbolTable* osymbols_;
  fst::ComposeFstOptions<Arc> copts_;
  PruneFct* prune_fct_;

 public:

  typedef typename boost::shared_ptr< fst::MutableFst<Arc> > FstPtr;

  AlignmentLatticesIterator(util::Data::const_iterator begin,
                            util::Data::const_iterator end,
                            const fst::Fst<fst::StdArc>& align_fst, 
                            const fst::SymbolTable* isymbols,
                            const fst::SymbolTable* osymbols) 
      : begin_(begin), end_(end), curr_(begin),
        align_fst_(align_fst), isymbols_(isymbols), osymbols_(osymbols),
        prune_fct_(NULL)
  {    
    copts_.gc_limit = 0;  // Cache only the last state for fastest copy.  
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
    fst::VectorFst<Arc> in_fst;
    fst::VectorFst<Arc> out_fst;
    util::ConvertStringToFst(curr_->first, *isymbols_, &in_fst);
    util::ConvertStringToFst(curr_->second, *osymbols_, &out_fst);
    fst::MutableFst<Arc>* aligned = new fst::VectorFst<Arc>();
    fst::ComposeFst<Arc> in_align(in_fst, align_fst_, copts_);
    fst::Compose(in_align, out_fst, aligned);
    if (prune_fct_ != NULL) {
      (*prune_fct_)(curr_->first, curr_->second, aligned);
    }
    return FstPtr(aligned);
  }
  
};

} } } // end namespaces

#endif
