#ifndef FSTR_CREATE_FEATURES_LATENT_ANNOTATION_FEATURES_H
#define FSTR_CREATE_FEATURES_LATENT_ANNOTATION_FEATURES_H

#include <string>
#include <vector>
#include <iostream>
// #include <boost/foreach.hpp>
#include <cmath>
#include <string>
#include <sstream>

namespace fstrain { namespace create { namespace features {

/**
 * @brief Represents all features with and without latent annotations.
 */
class LatentAnnotationFeatures {
  const std::string& str_;
  const char sep_;
  typedef std::vector<std::string> StringVec;
  // parts_ contains the positions of the string with their different
  // parts. TOOD: use matrix
  std::vector< std::vector<std::string> > parts_;
  StringVec vec_;
  StringVec::const_iterator it_;
  int maxnum_;
  std::string sep_str_;
  bool anchored_;
  
  // splits "a-c1-c3" into "a", "c1", "c3"; the first part may also
  // contain up to one "-" though
  StringVec SplitLa(const std::string& str, const char sep) {
    std::string::size_type sep_pos = str.find(sep);
    StringVec result;
    // "--c2" or "a|--c2" or "a|-"
    if(sep_pos == 0 || sep_pos == str.size() - 1 || str[sep_pos + 1] == sep) {
      sep_pos = str.find(sep, sep_pos + 1);
    }
    if(sep_pos == std::string::npos) {
      result.push_back(str);
    }
    else {
      result.push_back(str.substr(0, sep_pos));
      std::string la_part = str.substr(sep_pos + 1);
      if(!la_part.empty()) {
        std::stringstream ss(la_part);
        std::string la;
        while(std::getline(ss, la, sep)) {
          result.push_back(la);
        }
      }
    }
    return result;
  }

  /**
   * @brief Returns feature for start position and bit pattern
   * (e.g. from parts_="a-1-2 b-2-4 c-4-8" and start=1 and
   * bit_pattern=5 it returns "b-4 c-8")
   */
  std::string GetFeature(int start, int bit_pattern) {
    std::string feature;
    for(int i = start; i < parts_.size(); ++i) {
      const StringVec& v = parts_[i];
      bool first = true;
      for(std::size_t bitnum = 0; bitnum < maxnum_; ++bitnum) {	
        if(bit_pattern & (int)pow(2.0, (double)bitnum)){
          feature += (first ? "" : sep_str_) + v[bitnum];
          first = false;
        }
      }
      if(i < parts_.size() - 1) {
        feature += " ";
      }
    }
    return feature;
  }

 public:

  typedef StringVec::const_iterator const_iterator;

  /**
   * @param Only 'anchored' features fire, i.e. if they are not just latent annotations
   */
  LatentAnnotationFeatures(const std::string& str, 
                           bool anchored = false) : str_(str), 
                                                    sep_('-'), 
                                                    maxnum_(0),
                                                    anchored_(anchored) {
    sep_str_.resize(1);
    sep_str_[0] = sep_;
    std::string token;
    std::stringstream ss(str);
    while(std::getline(ss, token, ' ')) {
      StringVec v = SplitLa(token, sep_);
      parts_.push_back(v);      
      maxnum_ = std::max(maxnum_, (int)v.size());
    }
    for(std::size_t i = 0; i < parts_.size(); ++i) {
      while(parts_[i].size() < maxnum_) {
        parts_[i].push_back("??");
      }
    }
    int max_bit_pattern = (int)pow(2.0, (double)maxnum_) - 1; // all bits on      
    int max_bit = (int)pow(2.0, maxnum_ - 1.0); // highest bit
    for(int start = 0; start < parts_.size(); ++start) {
      int bit_pattern = max_bit_pattern;
      while(bit_pattern > 0) {
        // const bool bad_pattern = anchored_ && !(bit_pattern & max_bit);
        const bool bad_pattern = anchored_ && !(bit_pattern & 1);
        if(!bad_pattern) {
          const std::string feature = GetFeature(start, bit_pattern);
          vec_.push_back(feature);
        }
        --bit_pattern;
      }
    }
  }

  const_iterator begin() const { return vec_.begin();  }

  const_iterator end() const { return vec_.end();  }

};

} } } // end namespaces

#endif
