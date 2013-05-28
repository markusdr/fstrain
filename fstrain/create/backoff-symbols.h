// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: markus.dreyer@gmail.com (Markus Dreyer)
//

#ifndef FSTRAIN_CREATE_BACKOFF_SYMBOLS_H
#define FSTRAIN_CREATE_BACKOFF_SYMBOLS_H

#include <functional>
#include <string>
#include <boost/shared_ptr.hpp>
#include "fstrain/create/debug.h"
#include "fstrain/create/features/latent-annotations-util.h" // SplitMainFrom...

namespace fstrain { namespace create {

inline std::string::size_type FindSeparator(const std::string& sym, const char sep) {
  std::string::size_type sep_pos = sym.find(sep);
  if (sep_pos == 0) {
    FSTR_CREATE_EXCEPTION("No input dimension found in " << sym);
  }
  if (sep_pos == sym.length() - 1) {
    FSTR_CREATE_EXCEPTION("No output dimension found in " << sym);
  }
  if (sep_pos == std::string::npos) {
    FSTR_CREATE_EXCEPTION("Could not find separator in " << sym);
  }
  return sep_pos;
}

struct BackoffSymsFct : std::unary_function<std::string, std::string> {
  typedef boost::shared_ptr<BackoffSymsFct> Ptr;
  virtual std::string operator()(const std::string& sym) = 0;
  virtual const std::string& GetName() = 0;
  virtual ~BackoffSymsFct() {}
};

struct BackoffSyms_tlm : public BackoffSymsFct {
  std::string name;
  char in_out_sep;
  char la_sep;
  BackoffSyms_tlm() : name("tlm"), in_out_sep('|'), la_sep('-') {}
  const std::string& GetName() {return name;}
  std::string operator()(const std::string& sym0) {
    std::pair<std::string,std::string> both = features::SplitMainFromLatentAnnotations(sym0, la_sep);
    std::string sym = both.first;
    std::string la = both.second;
    std::string::size_type sep_pos = FindSeparator(sym, in_out_sep);
    std::string new_sym = sym.substr(sep_pos + 1);
    return new_sym + la;
  }
};

struct BackoffSyms_vc : public BackoffSymsFct {
  std::string name;
  char in_out_sep;
  char la_sep;
  BackoffSyms_vc() : name("vc"), in_out_sep('|'), la_sep('-') {}
  const std::string& GetName() {return name;}
  char ReduceChar(const char c) {
    const bool is_vowel = (c=='a'||c=='e'||c=='i'||c=='o'||c=='u');
    return is_vowel ? 'V' : c=='-' ? '-' : 'C';
  }
  std::string operator()(const std::string& sym0) {
    std::pair<std::string,std::string> both = features::SplitMainFromLatentAnnotations(sym0, la_sep);
    std::string sym = both.first;
    std::string la = both.second;
    std::string::size_type sep_pos = FindSeparator(sym, in_out_sep);
    if (!(sep_pos == 1 && sym.length() == 3)) {
      FSTR_CREATE_EXCEPTION("BackoffSyms_vc needs simple in" << in_out_sep
                            << "out chars, but got " << sym);
    }
    char left = ReduceChar(sym[0]);
    char right = ReduceChar(sym[2]);
    std::string new_sym;
    new_sym.resize(3);
    new_sym[0] = left;
    new_sym[1] = in_out_sep;
    new_sym[2] = right;
    return new_sym + la;
  }
};

struct BackoffSyms_id : public BackoffSymsFct {
  std::string name;
  char in_out_sep;
  char la_sep;
  BackoffSyms_id() : name("id"), in_out_sep('|'), la_sep('-') {}
  const std::string& GetName() {return name;}
  std::string operator()(const std::string& sym0) {
    std::pair<std::string,std::string> both = features::SplitMainFromLatentAnnotations(sym0, la_sep);
    std::string sym = both.first;
    std::string la = both.second;
    std::string::size_type sep_pos = FindSeparator(sym, in_out_sep);
    if (!(sep_pos == 1 && sym.length() == 3)) {
      FSTR_CREATE_EXCEPTION("BackoffSyms_id needs simple in" << in_out_sep
                            << "out chars, but got " << sym);
    }
    std::string new_sym;
    const char l = sym[0];
    const char r = sym[2];
    if (l == '-') {
      new_sym = "INS";
    }
    else if (r == '-') {
      new_sym = "DEL";
    }
    else if (l == r) {
      new_sym = "ID";
    }
    else {
      new_sym = "SUBST";
    }
    return new_sym + la;
  }
};

/**
 * @brief Factory function returns CreateBackoffSymsFct pointer
 */
inline BackoffSymsFct::Ptr CreateBackoffSymsFct(const std::string& name) {
  BackoffSymsFct::Ptr result;
  if (name == "tlm") {
    result = BackoffSymsFct::Ptr(new BackoffSyms_tlm());
  }
  else if (name == "vc") {
    result = BackoffSymsFct::Ptr(new BackoffSyms_vc());
  }
  else if (name == "id") {
    result = BackoffSymsFct::Ptr(new BackoffSyms_id());
  }
  // ... add new functions here
  else {
    FSTR_CREATE_EXCEPTION("Unknown backoff function " << name);
  }
  return result;
}

} } // end namespaces

#endif
