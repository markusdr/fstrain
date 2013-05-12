#ifndef FSTRAIN_CREATE_FEATURE_FUNCTIONS_H
#define FSTRAIN_CREATE_FEATURE_FUNCTIONS_H

#include <iostream>
#include <string>
#include <sstream>

namespace fstrain { namespace create { 

/**
 * @brief Adds a feature that fires whenever a character is emitted
 * (separately for input and output).
 */
template<class SetT>
void AddLengthPenalties(const std::string& symbol, 
                        const char sep_char,
                        const char eps_char,
                        SetT* featset) {
  std::string in, out;
  std::stringstream arcsym_ss(symbol);    
  std::getline(arcsym_ss, in, sep_char);
  std::getline(arcsym_ss, out, sep_char);
  if(in[0] != eps_char){
    featset->insert("*LENGTH_IN*");
  }
  if(out[0] != eps_char){
    featset->insert("*LENGTH_OUT*");
  }
}

} } // end namespaces

#endif
