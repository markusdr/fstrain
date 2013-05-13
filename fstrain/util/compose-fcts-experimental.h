#ifndef COMPOSE_FCTS_EXPERIMENTAL_H
#define COMPOSE_FCTS_EXPERIMENTAL_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/compose.h"
#include "fst/arcsort.h"
#include "fst/matcher.h"
#include <boost/shared_ptr.hpp>
#include "fstrain/util/print-fst.h"

#include "fstrain/util/compose-fcts.h"
#include "fstrain/util/determinize-in-place.h"
#include "fst/shortest-path.h"
#include "fst/prune.h"
#include "fst/map.h"
#include "fst/rmepsilon.h"
#include "fst/project.h"

namespace fstrain { namespace util {

/**
 * @brief result = prune(fst1 o proj_up) o fst2
 */
template<class Arc, class BaseComposeFct = DefaultComposeFct<Arc> >
class ProjUpPruneComposeFct : public ComposeFct<Arc> {
  boost::shared_ptr<const fst::Fst<Arc> > proj_up_;
  const fst::Fst<fst::StdArc>& prune_fst_;
  fst::StdArc::Weight weight_threshold_;
  SymbolTable* align_syms_; // just for dbg output
  BaseComposeFct base_compose_fct_;
 public:
  ProjUpPruneComposeFct(boost::shared_ptr<const fst::Fst<Arc> > proj_up, 
                        const fst::Fst<fst::StdArc>& prune_fst,
                        fst::StdArc::Weight weight_threshold,
                        BaseComposeFct base_compose_fct = BaseComposeFct()) 
      : proj_up_(proj_up), prune_fst_(prune_fst), weight_threshold_(weight_threshold),
        base_compose_fct_(base_compose_fct), align_syms_(NULL) {}  
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    using namespace fst;
    ComposeFstOptions<Arc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    assert(proj_up_);

    ComposeFst<Arc> up(fst1, *proj_up_);
    typedef WeightConvertMapper<Arc, StdArc> Map_AS;
    typedef WeightConvertMapper<StdArc,Arc> Map_SA;
    MapFst<Arc, StdArc, Map_AS> up_mapped(up, Map_AS()); 
    ComposeFst<StdArc> up_weighted(up_mapped, prune_fst_);
    VectorFst<StdArc> pruned;
    Prune(up_weighted, &pruned, weight_threshold_);
    //std::cerr << "<PRUNED>" << std::endl;
    //printAcceptor(&pruned, NULL, std::cerr);
    //std::cerr << "</PRUNED>" << std::endl;
    Map(&pruned, RmWeightMapper<StdArc>());
    ArcSort(&pruned, OLabelCompare<StdArc>());    
    //ArcSortFst<Arc, OLabelCompare<Arc> > up_sorted(up, OLabelCompare<Arc>());
    //base_compose_fct_(up_sorted, fst2, result);
    base_compose_fct_(MapFst<StdArc,Arc, Map_SA>(pruned, Map_SA()), fst2, result);
  }
  void SetAlignSymbols(SymbolTable* align_syms) {
    align_syms_ = align_syms;
  }
};

/**
 * @brief result = kbest(fst1 o proj_up) o fst2
 */
template<class Arc, class BaseComposeFct = DefaultComposeFct<Arc> >
class ProjUpKbestComposeFct : public ComposeFct<Arc> {
  boost::shared_ptr<const fst::Fst<Arc> > proj_up_;
  const fst::Fst<fst::StdArc>& weight_fst_;
  int kbest_;
  SymbolTable* align_syms_; // just for dbg output
  BaseComposeFct base_compose_fct_;
 public:
  ProjUpKbestComposeFct(boost::shared_ptr<const fst::Fst<Arc> > proj_up, 
                        const fst::Fst<fst::StdArc>& weight_fst,
                        int kbest,
                        BaseComposeFct base_compose_fct = BaseComposeFct()) 
      : proj_up_(proj_up), weight_fst_(weight_fst), kbest_(kbest),
        base_compose_fct_(base_compose_fct), align_syms_(NULL) {}  
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    using namespace fst;
    ComposeFstOptions<Arc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    assert(proj_up_);

    ComposeFst<Arc> up(fst1, *proj_up_);
    typedef WeightConvertMapper<Arc, StdArc> Map_AS;
    typedef WeightConvertMapper<StdArc,Arc> Map_SA;
    MapFst<Arc, StdArc, Map_AS> up_mapped(up, Map_AS()); 
    ComposeFst<StdArc> up_weighted(up_mapped, weight_fst_);
    VectorFst<StdArc> kbestd;
    ShortestPath(up_weighted, &kbestd, kbest_);
    RmEpsilon(&kbestd);
    Project(&kbestd, PROJECT_OUTPUT); // workaround for non-function problem
    Determinize(&kbestd);
    //std::cerr << "<KBESTD>" << std::endl;
    //printAcceptor(&kbestd, NULL, std::cerr);
    //std::cerr << "</KBESTD>" << std::endl;
    Map(&kbestd, RmWeightMapper<StdArc>());
    ArcSort(&kbestd, OLabelCompare<StdArc>());    
    //ArcSortFst<Arc, OLabelCompare<Arc> > up_sorted(up, OLabelCompare<Arc>());
    //base_compose_fct_(up_sorted, fst2, result);
    base_compose_fct_(MapFst<StdArc,Arc, Map_SA>(kbestd, Map_SA()), fst2, result);
  }
  void SetAlignSymbols(SymbolTable* align_syms) {
    align_syms_ = align_syms;
  }
};

} } // end namespace

#endif
