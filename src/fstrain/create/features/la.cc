#include <string>
#include <iostream>
#include <algorithm>
#include "fstrain/create/features/feature-set.h"

// #include "fstrain/create/get-features.cc" // include source
#include "fstrain/create/features/latent-annotation-features.h"

namespace fstrain { namespace create { namespace features {

extern "C" {
  
  void ExtractFeatures(const std::string& window, 
                       IFeatureSet* featset) {
    // GetFeatures(window, featset);
    LatentAnnotationFeatures f(window);
    for(LatentAnnotationFeatures::const_iterator it = f.begin(); 
        it != f.end(); ++it) {
      featset->insert(*it);
    }

  }

}

} } } // end namespaces
