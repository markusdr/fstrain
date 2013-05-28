#include <string>
#include <iostream>
// #include <algorithm>

#include "fstrain/create/features/feature-set.h"
#include "fstrain/create/features/latent-annotation-features.h"

namespace fstrain { namespace create { namespace features {

extern "C" {

  // la1 creates only 'anchored' features, e.g not "c1-c2" from
  // "a|x-c1-c2" because it is not anchored at an actual character.
  void ExtractFeatures(const std::string& window,
                       IFeatureSet* featset) {
    const bool anchored = true;
    LatentAnnotationFeatures f(window, anchored);
    for(LatentAnnotationFeatures::const_iterator it = f.begin();
        it != f.end(); ++it) {
      featset->insert(*it);
    }

  }

}

} } } // end namespaces
