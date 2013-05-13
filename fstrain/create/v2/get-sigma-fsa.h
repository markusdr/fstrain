#ifndef FSTRAIN_CREATE_V2_GET_SIGMA_FSA_H
#define FSTRAIN_CREATE_V2_GET_SIGMA_FSA_H

#include "fst/symbol-table.h"
#include "fst/mutable-fst.h"

namespace fstrain { namespace create {

/**
 * @brief Constructs a simple FSA that accepts one symbol which may be
 * any symbol (accepts sigma).
 */
template<class Arc>
void GetSigmaFsa(const fst::SymbolTable& syms,
                 fst::MutableFst<Arc>* result) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  StateId s1 = result->AddState();
  StateId s2 = result->AddState();
  result->SetStart(s1);
  result->SetFinal(s2, Weight::One());
  SymbolTableIterator sit(syms);
  sit.Next(); // ignore eps
  for(; !sit.Done(); sit.Next()) {
    result->AddArc(s1, 
                   Arc(sit.Value(), sit.Value(), Weight::One(), s2));
  }
}

} } // end namespace


#endif
