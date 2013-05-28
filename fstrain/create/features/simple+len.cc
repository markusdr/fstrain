#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/features/feature-set.h"
#include "fstrain/create/feature-functions.h"
#include <ctype.h> // isblank

namespace fstrain { namespace create { namespace features {

extern "C" {

  const char sep_char = '|';
  const char eps_char = '-';

  /**
   * Adds simple features and length penalty features, e.g. for "a|x
   * b|-" (history "a|x" with outgoing arc "b|-") it adds "a|x b|-",
   * "b|-", LENGTH_IN
   */
  void ExtractFeatures(const std::string& window,
                       IFeatureSet* featset) {
    std::string feature_str;
    std::string::const_iterator s = window.begin();
    bool first = true;
    do {
      if (!first) {
        ++s;
      }
      feature_str = std::string(s, window.end());
      // std::cerr << "feature_str=" << feature_str << std::endl;
      featset->insert(feature_str);
      first = false;
    } while ((s = find_if (s, window.end(), isblank)) != window.end());

    // Note these are only unigram features:
    if (!feature_str.empty()) {
      AddLengthPenalties(feature_str, sep_char, eps_char, featset);
    }
  }

}

} } } // end namespaces
