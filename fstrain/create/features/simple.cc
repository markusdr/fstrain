#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/features/feature-set.h"
#include <ctype.h> // isblank

namespace fstrain { namespace create { namespace features {

extern "C" {

  void ExtractFeatures(const std::string& window,
                       IFeatureSet* featset) {
    featset->insert(window);
    std::string::const_iterator s = window.begin();
    while ((s = find_if (s, window.end(), isblank)) != window.end()) {
      std::string backoff_feat(++s, window.end());
      featset->insert(backoff_feat);
    }
  }

}

} } } // end namespaces
