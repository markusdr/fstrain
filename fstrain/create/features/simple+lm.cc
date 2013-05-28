#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/features/feature-set.h"
#include <ctype.h> // isblank

namespace fstrain { namespace create { namespace features {

extern "C" {

  std::string HideInput(const std::string& s) {
    const char sep = '|';
    std::stringstream output;
    std::stringstream ss(s);
    std::string token;
    bool first = true;
    while (ss >> token) {
      std::string::size_type sep_pos = token.find(sep);
      if (sep_pos != std::string::npos) {
        if (!first) {
          output << " ";
        }
        output << "?" << token.substr(sep_pos);
        first = false;
      }
    }
    return sep;
  }

  void ExtractFeatures0(const std::string& window,
                        IFeatureSet* featset) {
    featset->insert(window);
    std::string::const_iterator s = window.begin();
    while ((s = find_if (s, window.end(), isblank)) != window.end()) {
      std::string backoff_feat(++s, window.end());
      featset->insert(backoff_feat);
    }
  }

  void ExtractFeatures(const std::string& window,
                       IFeatureSet* featset) {
    ExtractFeatures0(window, featset);
    const std::string target_lm = HideInput(window);
    ExtractFeatures0(target_lm, featset);
  }
}

} } } // end namespaces
