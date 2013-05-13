#ifndef FSTR_CREATE_BACKOFF_UTIL_H
#define FSTR_CREATE_BACKOFF_UTIL_H

#include "fstrain/create/backoff-symbols.h"
#include "fstrain/create/features/extract-features.h"
#include <string>
#include <cstdlib>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>

namespace fstrain { namespace create {

/**
 * Parses a string that describes the various backoffs.
 * 
 * @param str Backoff description string, e.g. "vc,tlm",
 * "vc/la,tlm/la", "vc/la/3,tlm/la/2", which contains the different
 * backoffs separated by comma, and each backoff may specify an
 * extraction method (to extract features from backed-off ngram),
 * otherwise "simple" is used, and an ngram order, see
 * extract-features.h
 */
void ParseBackoffDescription(const std::string& str, 
                             const std::size_t default_ngram_order,
                             std::vector<boost::tuple<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr, std::size_t> >* vec) {
  typedef features::ExtractFeaturesFct::Ptr ExtractFctPtr;
  std::string name;    
  std::stringstream ss(str); // e.g. "tlm/collapsed,vc"
  while(std::getline(ss, name, ',')) {
    std::vector<string> tokens;
    boost::split(tokens, name, boost::is_any_of("/")); // split "vc/la/3"
    const std::string backoff_name = tokens[0];
    const std::string feats_libname = tokens.size() > 1 ? tokens[1] : "simple";
    const std::size_t ngram_order = 
        tokens.size() > 2 
        ? boost::lexical_cast<std::size_t>(tokens[2]) 
        : default_ngram_order;

    ExtractFctPtr feats_fct(new features::ExtractFeaturesFctPlugin(feats_libname));
    std::cerr << "# Using backoff-symbols function " << backoff_name
              << " (" << feats_libname << ") with ngram order " << ngram_order << std::endl;
    vec->push_back(boost::make_tuple(BackoffSymsFct::Ptr(CreateBackoffSymsFct(backoff_name)),
                                     feats_fct,
                                     ngram_order));
  }

}

} } // end namespaces

#endif
