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
#ifndef FSTRAIN_UTIL_COMPOSE_FCTS_H
#define FSTRAIN_UTIL_COMPOSE_FCTS_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/compose.h"
#include "fst/arcsort.h"
#include "fst/matcher.h"
#include <boost/shared_ptr.hpp>
#include "fstrain/util/print-fst.h"
 
namespace fstrain { namespace util {

template<class Arc>
struct ComposeFct {
  virtual ~ComposeFct() {}
  virtual void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                          fst::MutableFst<Arc>* result) = 0;  
};

/**
 * @brief result = fst1 o fst2
 */
template<class Arc>
struct DefaultComposeFct : public ComposeFct<Arc> {
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    fst::Compose(fst1, fst2, result);
  }
};

/**
 * @brief result = fst1 o fst2, where fst1 may have phi labels
 */
template<class Arc>
struct ComposePhiLeftFct : public ComposeFct<Arc> {
  int64 phi_label_;
  ComposePhiLeftFct(int64 phi_label) : phi_label_(phi_label) {}
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    using namespace fst;
    ArcSortFst<Arc, OLabelCompare<Arc> > fst1_sorted(fst1, OLabelCompare<Arc>());
    typedef Fst<Arc> FST;
    typedef PhiMatcher< SortedMatcher<FST> > SM;
    ComposeFstOptions<Arc, SM> opts;
    opts.gc_limit = 0;
    opts.matcher1 = new SM(fst1_sorted, MATCH_OUTPUT, phi_label_);
    opts.matcher2 = new SM(fst2, MATCH_NONE);
    ComposeFst<Arc> composed(fst1_sorted, fst2, opts);
    *result = composed;
    Connect(result);
  }
};

/**
 * @brief result = fst1 o fst2, where fst2 may have phi labels
 */
template<class Arc>
struct ComposePhiRightFct : public ComposeFct<Arc> {
  int64 phi_label_;
  ComposePhiRightFct(int64 phi_label) : phi_label_(phi_label) {}
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    using namespace fst;
    ArcSortFst<Arc, ILabelCompare<Arc> > fst2_sorted(fst2, ILabelCompare<Arc>());
    typedef Fst<Arc> FST;
    typedef PhiMatcher< SortedMatcher<FST> > SM;
    ComposeFstOptions<Arc, SM> opts;
    opts.gc_limit = 0;
    opts.matcher1 = new SM(fst1, MATCH_NONE);
    opts.matcher2 = new SM(fst2_sorted, MATCH_INPUT, phi_label_);
    ComposeFst<Arc> composed(fst1, fst2_sorted, opts);
    *result = composed;
    Connect(result);
  }
};

/**
 * @brief result = (fst1 o proj_up) o fst2
 */
template<class Arc, class BaseComposeFct = DefaultComposeFct<Arc> >
class ProjUpComposeFct : public ComposeFct<Arc> {
  boost::shared_ptr<const fst::Fst<Arc> > proj_up_;
  BaseComposeFct base_compose_fct_;
 public:
  ProjUpComposeFct(boost::shared_ptr<const fst::Fst<Arc> > proj_up, 
                   BaseComposeFct base_compose_fct = BaseComposeFct()) 
      : proj_up_(proj_up), base_compose_fct_(base_compose_fct) {}
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    fst::ComposeFstOptions<Arc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    assert(proj_up_);
    fst::ComposeFst<Arc> up(fst1, *proj_up_);
    typedef fst::OLabelCompare<Arc> OLabelComp;
    fst::ArcSortFst<Arc, OLabelComp> up_sorted(up, OLabelComp());
    base_compose_fct_(up_sorted, fst2, result);
  }
};


/**
 * @brief result = fst1 o (proj_down o fst2)
 */
template<class Arc, class BaseComposeFct = DefaultComposeFct<Arc> >
class ProjDownComposeFct : public ComposeFct<Arc> {
  boost::shared_ptr<const fst::Fst<Arc> > proj_down_;
  BaseComposeFct base_compose_fct_;
 public:
  ProjDownComposeFct(boost::shared_ptr<const fst::Fst<Arc> > proj_down,
                     BaseComposeFct base_compose_fct = BaseComposeFct()) 
      : proj_down_(proj_down), base_compose_fct_(base_compose_fct) {}
  void operator()(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, 
                  fst::MutableFst<Arc>* result) {
    fst::ComposeFstOptions<Arc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    assert(proj_down_);
    fst::ComposeFst<Arc> down(*proj_down_, fst2);
    typedef fst::ILabelCompare<Arc> ILabelComp;
    fst::ArcSortFst<Arc, ILabelComp> down_sorted(down, ILabelComp());
    base_compose_fct_(fst1, down_sorted, result);
  }
};

} }

#endif
