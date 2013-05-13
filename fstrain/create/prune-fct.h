#ifndef FSTRAIN_CREATE_PRUNE_FCT_H
#define FSTRAIN_CREATE_PRUNE_FCT_H

#include <string>
#include "fst/mutable-fst.h"
#include "fst/fst.h"
#include "fst/prune.h"

namespace fstrain { namespace create {

struct PruneFct {
  virtual ~PruneFct() {}
  virtual void operator()(const std::string& in_str, const std::string& out_str, 
                          fst::MutableFst<fst::StdArc>* f) = 0;
};

struct DefaultPruneFct : public PruneFct {
  fst::StdArc::Weight weight_threshold_;
  double state_factor_;
  virtual ~DefaultPruneFct() {}
  DefaultPruneFct(fst::StdArc::Weight weight_threshold,
                  double state_factor) 
      : weight_threshold_(weight_threshold), state_factor_(state_factor) {}
  virtual void operator()(const std::string& in_str, const std::string& out_str, 
                          fst::MutableFst<fst::StdArc>* f) {
    fst::StdArc::StateId state_threshold = 
        (fst::StdArc::StateId)(std::max(in_str.length(), out_str.length()) * state_factor_);
    std::cerr << f->NumStates() << " => ";
    fst::Prune(f, weight_threshold_, state_threshold);
    std::cerr << f->NumStates() << std::endl;
  }
};

}} // end namespaces

#endif
