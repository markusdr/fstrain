#ifndef FSTRAIN_CREATE_FEATURES_EXTRACT_FEATURES_H
#define FSTRAIN_CREATE_FEATURES_EXTRACT_FEATURES_H

#include <string>
#include "fstrain/create/debug.h"
#include "fstrain/create/features/feature-set.h"
#include "fstrain/util/load-library.h"
#include <ctype.h> // isblank
#include <boost/shared_ptr.hpp>

namespace fstrain { namespace create { namespace features {

// interface for function object
struct ExtractFeaturesFct {
  typedef boost::shared_ptr<ExtractFeaturesFct> Ptr;
  virtual ~ExtractFeaturesFct() {}
  virtual void operator()(const std::string& window,
                          IFeatureSet* featset) = 0;
};

struct ExtractFeaturesFct_Simple : public ExtractFeaturesFct {
  const std::string prefix;
  ExtractFeaturesFct_Simple() {}
  ExtractFeaturesFct_Simple(const std::string& prefix_) : prefix(prefix_ + ":") {}
  virtual ~ExtractFeaturesFct_Simple() {}
  virtual void operator()(const std::string& window,
                          IFeatureSet* featset) {
    featset->insert(prefix + window);
    std::string::const_iterator s = window.begin();
    while ((s = find_if (s, window.end(), isblank)) != window.end()) {
      std::string backoff_feat(++s, window.end());
      featset->insert(prefix + backoff_feat);
    }
  }
};

// implementation which loads its code at runtime
struct ExtractFeaturesFctPlugin : public ExtractFeaturesFct {
  ExtractFeaturesFctPlugin(const std::string& libname) {
    LoadExtractFeaturesFunction(libname);
  }
  virtual ~ExtractFeaturesFctPlugin() {}
  void operator()(const std::string& window,
                  IFeatureSet* featset) {
    loaded_fct_(window, featset);
  }
 private:

  void LoadExtractFeaturesFunction(const std::string& libname) {
    using namespace fstrain::util;
    std::cerr << "# Loading feature extraction library '"
              << libname << "'" << std::endl;
    my_lib_t hMyLib = NULL;
    if (!(hMyLib = LoadLib(std::string(libname + ".so").c_str()))) {
      FSTR_CREATE_EXCEPTION(dlerror());
    }
    if (!(loaded_fct_ = (ExtractFeatures_t)LoadProc(hMyLib, "ExtractFeatures"))) {
      FSTR_CREATE_EXCEPTION(dlerror());
    }
  }

  typedef void (*ExtractFeatures_t)(const std::string&,
                                    fstrain::create::features::IFeatureSet*);
  ExtractFeatures_t loaded_fct_;
};

} } } // end namespaces

#endif
