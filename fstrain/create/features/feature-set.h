#ifndef FSTRAIN_CREATE_FEATURES_FEATURE_SET_H
#define FSTRAIN_CREATE_FEATURES_FEATURE_SET_H

namespace fstrain { namespace create { namespace features {

struct IFeatureSet {
  virtual ~IFeatureSet() {}
  virtual void insert(const std::string&) = 0;
};

} } } // end namespace

#endif
