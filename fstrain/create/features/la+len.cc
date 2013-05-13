#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/debug.h"
#include "fstrain/create/features/feature-set.h"
#include "fstrain/create/feature-functions.h" // AddLengthPenalties

// #include "fstrain/create/get-features.cc" // include source
#include "fstrain/create/features/latent-annotation-features.h"
#include "fstrain/create/features/latent-annotations-util.h" // SplitMainFromLatentAnnotations

namespace fstrain { namespace create { namespace features {

extern "C" {
  
  /**
   * @brief Extracts all features with and without the different
   * subsets of latent annotations.
   */
  void ExtractFeatures(const std::string& window, 
                       IFeatureSet* featset) {
    // GetFeatures(window, featset);
    LatentAnnotationFeatures f(window);
    for(LatentAnnotationFeatures::const_iterator it = f.begin(); 
        it != f.end(); ++it) {
      featset->insert(*it);
    }

    std::size_t start_last_symbol = 0;
    std::string::size_type last_blank = window.find_last_of(' ');
    if(last_blank != std::string::npos) {
      if(last_blank == window.size() - 1) {
        FSTR_CREATE_EXCEPTION("window " << window << " end with blank");
      }
      start_last_symbol = last_blank + 1;
    }
    std::string sym = window.substr(start_last_symbol);// e.g. "a|x-c1-g2"
    const char la_sep = '-';
    const char in_out_sep = '|';
    const char eps_char = '-';
    std::pair<std::string,std::string> both = SplitMainFromLatentAnnotations(sym, la_sep); // "a|x", "-c1-g2"
    AddLengthPenalties(both.first, in_out_sep, eps_char, featset);

  }

}

} } } // end namespaces
