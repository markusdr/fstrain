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
/**
 * $Id: expectations.h 6315 2009-11-28 20:37:51Z markus $
 *
 * @brief
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_CORE_EXPECTATIONS_H
#define FSTRAIN_CORE_EXPECTATIONS_H

#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <stdexcept>
#include "fstrain/core/util.h"
#include "fst/compat.h"
#include "fstrain/core/neg-log-of-signed-num.h"

namespace fstrain { namespace core {

class MDExpectations {

 public:

  typedef std::map<int, NeglogNum> Container;
  typedef Container::const_iterator const_iterator;
  typedef Container::iterator iterator;
  typedef Container::size_type size_type;
  typedef Container::key_type key_type;       // type of key
  typedef Container::mapped_type mapped_type; // typed of mapped data
  typedef Container::value_type value_type;   // type of key/value pair

  MDExpectations() : count_(1) {}

  MDExpectations(const MDExpectations& other) : count_(1) {
    expectations_ = other.get();
  }

  virtual ~MDExpectations() {}

  /**
   * Inserts a set of expectations
   * @param s The string that describes the expectations, e.g. [0=0.69123,12=-3.4524]
   */
  void insert(const std::string& s) {
    char delims[] = ",";
    char* token = 0;
    token = strtok( (char*)s.c_str(), delims );
    while( token != 0 ) { // e.g. 1=0.70711
      std::string entryStr = token;
      std::string::size_type equalSignPos = entryStr.find("=");
      if(equalSignPos != std::string::npos){
        std::string indexStr = entryStr.substr(0,equalSignPos);
        int index = stringToUnsignedLong(indexStr);
        std::string vStr = entryStr.substr(equalSignPos+1);
        double v = stringToDouble(vStr);
        insert(index, NeglogNum(v));
      }
      else{
        throw std::runtime_error("Format error in expectations");
      }
      token = strtok(0, delims);
    }
  }

  NeglogNum operator[] (unsigned index) const {
    const_iterator found = expectations_.find(index);
    if(found != expectations_.end()){
      return found->second;
    }
    return NeglogNum(kPosInfinity);
  }

  NeglogNum& operator[] (unsigned index) {
    return expectations_[index];
  }

  void update(int i, NeglogNum d) {
    expectations_.erase(i);
    if(d != kPosInfinity){
      expectations_.insert(std::make_pair(i,d));
    }
  }

  template<class InputIterator>
  void insert(InputIterator first, InputIterator last) {
    expectations_.insert(first, last);
  }

  void insert(int i, NeglogNum d) {
    if(d == kPosInfinity){
      expectations_.erase(i);
    }
    else {
      expectations_.insert(std::make_pair(i,d));
    }
  }

  iterator begin() { return expectations_.begin(); }

  iterator end() { return expectations_.end(); }

  void clear() { return expectations_.clear(); }

  const_iterator find(const int& index) const  { return expectations_.find(index); }

  iterator find(const int& index)  { return expectations_.find(index); }

  const_iterator begin() const { return expectations_.begin(); }

  const_iterator end() const { return expectations_.end(); }

  unsigned size() const { return expectations_.size(); }

  std::ostream& print(std::ostream& out) const {
    if(expectations_.size()){
      out << "[";
      bool start = true;
      for(const_iterator it = expectations_.begin(); it != expectations_.end(); ++it){
        if(!start){
          out << ",";
        }
        out << it->first << "=" << it->second;
        start = false;
      }
      out << "]";
    }
    return out;
  }

  // TODO: take static functions out of class

  static void timesValue(const MDExpectations& e,
                         const NeglogNum& d,
                         MDExpectations& result) {
    for(const_iterator it = e.begin(); it != e.end(); ++it){
      result.insert(it->first, NeglogTimes(it->second, d));
    }
  }

  static void divideBy(const MDExpectations& e,
                       const NeglogNum& d,
                       MDExpectations& result) {
    for(const_iterator it = e.begin(); it != e.end(); ++it){
      result.insert(it->first, NeglogDivide(it->second, d));
    }
  }

  unsigned getCount() const { return count_; }

  unsigned incCount() { return __sync_add_and_fetch(&count_, 1); }
  unsigned decCount() {
    if(count_ > 0)
      __sync_sub_and_fetch(&count_, 1);
    return count_;
  }

//  unsigned incCount(){ return ++count_; }
//  unsigned decCount(){
//    if(count_ > 0){
//      --count_;
//    }
//    return count_;
//  }

 protected:

  const Container& get() const { return expectations_; }

 private:

  void operator=(const MDExpectations&); // disallowed

  Container expectations_;
  unsigned count_;

}; // end class MDExpectations

inline bool ApproxEqual(double d1, double d2, double delta) {
  return d1 <= d2 + delta && d2 <= d1 + delta;
}

inline bool ApproxEqual(NeglogNum d1, NeglogNum d2,
                        double delta) {
  return d1.sign_x == d2.sign_x && ApproxEqual(d1.lx, d2.lx, delta);
}

inline bool ApproxEqual(const MDExpectations& e1,
                        const MDExpectations& e2,
                        double delta) {
  if (e1.size() != e2.size()) {
    return false;
  }
  MDExpectations::const_iterator found;
  for(MDExpectations::const_iterator i1 = e1.begin(); i1 != e1.end(); ++i1) {
    found = e2.find(i1->first);
    if (found == e2.end() || !ApproxEqual(i1->second, found->second, delta)) {
      return false;
    }
  }
  return true;
}

} } // end namespace fstrain/core

#endif

