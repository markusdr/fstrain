#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/features/feature-set.h"
#include <ctype.h> // isblank

namespace fstrain { namespace create { namespace features {

extern "C" {

  void ExtractFeatures_(const std::string& window,
                        IFeatureSet* featset) {
    featset->insert(window);
    std::string::const_iterator s = window.begin();
    while((s = find_if(s, window.end(), isblank)) != window.end()){
      std::string backoff_feat(++s, window.end());
      featset->insert(backoff_feat);
    }
  }

  std::string CollapseString(const std::string& str) {
    std::string collapsed;
    collapsed.reserve(str.size());
    for(std::size_t i = 0; i < str.size(); ++i) {
      if(str[i] == '-') {
        ++i; // ignore next blank in "- a" or "a - x"
      }
      else if(i == str.size() - 2 && str[i] == ' ' && str[i+1] == '-') { // "a -"
        break;
      }
      else {
        collapsed.push_back(str[i]);
      }
    }
    return collapsed;
  }

  void ExtractFeatures(const std::string& window,
                       IFeatureSet* featset) {
    ExtractFeatures_(window, featset);
    std::string collapsed = CollapseString(window);
    if(!collapsed.empty() && collapsed != window) {
      ExtractFeatures_(collapsed, featset);
    }
  }

} // end extern

} } } // end namespaces
