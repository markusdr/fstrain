#ifndef FSTR_CREATE_FEATURES_LATENT_ANNOTATIONS_UTIL_H
#define FSTR_CREATE_FEATURES_LATENT_ANNOTATIONS_UTIL_H

namespace fstrain { namespace create { namespace features {

/**
 * @brief Splits into the main and the latent-annotations part, e.g "a|x-c2" => ("a|x", "-c2").
 */
inline std::pair<std::string,std::string> SplitMainFromLatentAnnotations(const std::string& sym, 
                                                                         const char la_sep) {
  std::string::size_type la_sep_pos = sym.find(la_sep);
  // "-|x" or "a|-" or the first hyphen in "a|--c1"
  const bool search_for_next_sep = 
      la_sep_pos == 0 || la_sep_pos == sym.size() - 1 || sym[la_sep_pos + 1] == la_sep;
  if(search_for_next_sep) {
    la_sep_pos = sym.find(la_sep, la_sep_pos + 1);
  }
  if(la_sep_pos == std::string::npos) {
    return std::make_pair(sym, "");
  }
  else { // e.g. sym="a|--c1"
    return std::make_pair(sym.substr(0, la_sep_pos), 
                          sym.substr(la_sep_pos));
  }
}

} } } // end namespaces

#endif
