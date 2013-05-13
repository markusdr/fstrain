#ifndef FSTR_UTIL_INTERSECT_VEC_H
#define FSTR_UTIL_INTERSECT_VEC_H

#include <vector>
#include "fst/intersect.h"
#include "fst/minimize.h"
#include "fst/mutable-fst.h"
#include "fst/rmepsilon.h"
#include "fst/vector-fst.h"
#include "fstrain/util/determinize-in-place.h"

namespace fstrain { namespace util {

template<class Arc>
void MyIntersect(const fst::Fst<Arc>& ifst1, 
                 const fst::Fst<Arc>& ifst2,
                 fst::MutableFst<Arc>* ofst,
                 bool do_determinize) {
  fst::Intersect(ifst1, ifst2, ofst);      
  if(do_determinize){
    fst::RmEpsilon(ofst);
    fstrain::util::Determinize(ofst);
    fst::Minimize(ofst);
  }  
}

template<class Arc>
void Intersect_vec(const std::vector<fst::Fst<Arc>*>& ifsts, 
                   fst::MutableFst<Arc>* ofst, 
                   bool determinizeIntermediate = true) {
  if(ifsts.size() == 0){
    return;
  }
  if(ifsts.size() == 1){
    *ofst = *(ifsts[0]);
    return;
  }
  typedef fst::ArcSortFst<Arc, fst::OLabelCompare<Arc> > MyArcSortFst;
  MyIntersect(MyArcSortFst(*ifsts[0], fst::OLabelCompare<Arc>()), 
              *ifsts[1], 
              ofst,
              determinizeIntermediate);
  for(int i = 2; i < ifsts.size(); ++i){
    MyArcSortFst ofst_sorted(*ofst, fst::OLabelCompare<Arc>());
    MyIntersect(ofst_sorted, *ifsts[i], ofst, determinizeIntermediate);      
  }
}

} } // end namespaces


#endif
