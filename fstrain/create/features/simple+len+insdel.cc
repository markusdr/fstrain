#include <string>
#include <iostream>
#include <algorithm>
#include <ctype.h> // isblank

#include "fstrain/create/features/feature-set.h"
#include "fstrain/create/feature-functions.h"
#include "fstrain/create/debug.h"

namespace fstrain { namespace create { namespace features {

extern "C" {

  const char sep_char = '|';
  const char eps_char = '-';

  /**
   * Adds simple features and length penalty features, e.g. for "a|x
   * b|-" (history "a|x" with outgoing arc "b|-") it adds "a|x b|-",
   * "b|-", LENGTH_IN, DEL
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
      featset->insert(feature_str);
      first = false;
    } while ((s = find_if (s, window.end(), isblank)) != window.end());

    // Note the following are only unigram features:

    if (!feature_str.empty()) {
      AddLengthPenalties(feature_str, sep_char, eps_char, featset);
    }
    if (!feature_str.empty()) {
      FSTR_CREATE_EXCEPTION("unimplemented");
      // AddInsDelFeatures(feature_str, sep_char, eps_char, featset);
    }
  }

}

} } } // end namespaces
