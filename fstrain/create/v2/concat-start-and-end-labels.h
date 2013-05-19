#ifndef FSTRAIN_CREATE_CONCAT_START_AND_END_LABELS_H
#define FSTRAIN_CREATE_CONCAT_START_AND_END_LABELS_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/concat.h"

namespace fstrain { namespace create {

namespace nsConcatStartAndEndLabelsUtil {

// Helper function creates an FST with one arc from start to end
// state.
template<class Arc>
void CreateOneArcFst(const int64 ilabel,
                     const int64 olabel,
                     fst::MutableFst<Arc>* fst) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  StateId s1 = fst->AddState();
  StateId s2 = fst->AddState();
  fst->SetStart(s1);
  fst->AddArc(s1, Arc(ilabel, olabel, Weight::One(), s2));
  fst->SetFinal(s2, Weight::One());
}

} // end namespace

/**
 * @brief Concats start and end labels to the FST.
 */
template<class Arc>
void ConcatStartAndEndLabels(const int64 kStartLabel, const int64 kEndLabel,
                             bool input_eps, bool output_eps,
                             fst::MutableFst<Arc>* fst) {
  using namespace fst;
  using namespace nsConcatStartAndEndLabelsUtil;
  VectorFst<Arc> start_fst;
  int64 ilabel = input_eps ? 0 : kStartLabel;
  int64 olabel = output_eps ? 0 : kStartLabel;
  CreateOneArcFst(ilabel, olabel, &start_fst);
  VectorFst<Arc> end_fst;
  ilabel = input_eps ? 0 : kEndLabel;
  olabel = output_eps ? 0 : kEndLabel;
  CreateOneArcFst(ilabel, olabel, &end_fst);
  std::cerr << "Concat in v2" << std::endl; // TEST
  Concat(start_fst, fst);
  Concat(fst, end_fst);
  RmEpsilon(fst);
}

} } // end namespaces

#endif
