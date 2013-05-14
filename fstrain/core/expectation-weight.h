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

#ifndef FSTRAIN_CORE_EXPECTATION_WEIGHT_H
#define FSTRAIN_CORE_EXPECTATION_WEIGHT_H

#include <stdexcept>
#include <string>
#include <boost/functional/hash.hpp>
#include "fst/float-weight.h"
#include "fstrain/core/expectations.h"
#include "fstrain/core/neg-log-of-signed-num.h"
#include "fstrain/core/util.h"
#include "fstrain/core/debug.h"
#include "fstrain/util/add-maps.h"
#include "fstrain/util/multiplied-map.h"

namespace fst {

typedef LogWeightTpl<double> MDExpectationWeightBase;

class MDExpectationWeight : public MDExpectationWeightBase {

 public:

  typedef fstrain::core::MDExpectations MDExpectations;

  typedef MDExpectationWeight ReverseWeight;

  MDExpectationWeight() : MDExpectationWeightBase(), expectations_(0) {}

  MDExpectationWeight(double f) : MDExpectationWeightBase(f), expectations_(0) {}

  MDExpectationWeight(const MDExpectationWeight& other)
      : MDExpectationWeightBase(other.Value()), expectations_(other.expectations_) {
    if(expectations_){
      expectations_->incCount();
    }
  }

  MDExpectationWeight& operator= (const MDExpectationWeight& other) {
    MDExpectations* const old = expectations_;
    expectations_ = other.expectations_;
    if(expectations_ != 0)
      expectations_->incCount();
    if (old != 0 && old->decCount() == 0) {
      delete old;
    }
    SetValue(other.Value());
    return *this;
  }

  virtual ~MDExpectationWeight() {
    if(expectations_ != 0 && expectations_->decCount() == 0){
      delete expectations_;
      expectations_ = 0;
    }
  }

  // void SetValue(double v) { value_ = v; }

  const MDExpectations& GetMDExpectations() const {
    if(expectations_ == 0){
      return getEmptyStaticMDExpectations();
    }
    return *expectations_;
  }

  MDExpectations& GetMDExpectations() {
    if(expectations_ == 0){
      expectations_ = new MDExpectations();
    }
    if (expectations_->getCount() > 1) {
      MDExpectations* d = new MDExpectations(*expectations_);
      expectations_->decCount();
      expectations_ = d;
    }
    assert(expectations_->getCount() == 1);
    return *expectations_;
  }

  static const MDExpectationWeight Zero() {
    return MDExpectationWeight(fstrain::core::kPosInfinity); }

  static const MDExpectationWeight One() {
    return MDExpectationWeight(0.0F); }

  static const std::string &Type() {
    // static const string type = "expectation" + FloatWeightTpl<double>::GetPrecisionString();
    static const std::string type = "expectation";
    return type;
  }

  static const MDExpectationWeight NoWeight() {
    return MDExpectationWeight(FloatLimits<double>::NumberBad()); }

  bool Member() const {
    // First part fails for IEEE NaN
    bool is_member = Value() == Value()
        && Value() != FloatLimits<double>::PosInfinity();
    if(!is_member){
      return false;
    }
    const MDExpectations& e = GetMDExpectations();
    for(MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it){
      const double neglog_val = it->second.lx;
      bool is_member = neglog_val == neglog_val
          && neglog_val != FloatLimits<double>::NegInfinity();
      if(!is_member){
        return false;
      }
    }
  }

  MDExpectationWeight Quantize(double delta = kDelta) const {
    MDExpectationWeight result;
    result.SetValue(MDExpectationWeightBase(Value()).Quantize(delta).Value());
    const MDExpectations& e = GetMDExpectations();
    MDExpectations& result_e = result.GetMDExpectations();
    for(MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it){
      result_e.insert(it->first, NeglogQuantize(it->second, delta));
    }
    return result;
  }

  MDExpectationWeight Reverse() const {
    return *this;
  }

  static uint64 Properties() {
    return kLeftSemiring | kRightSemiring | kCommutative;
  }

  istream &Read(istream &strm) {
    using namespace fstrain::core;
    double val;
    ReadType(strm, &val);
    SetValue(val);
    int64 size0;
    ReadType(strm, &size0);
    GetMDExpectations().clear();
    MDExpectations::size_type size = static_cast<MDExpectations::size_type>(size0);
    for(MDExpectations::size_type i = 0; i < size; ++i){
      int64 index;
      double expectation;
      ReadType(strm, &index);
      ReadType(strm, &expectation);
      GetMDExpectations().insert(index, NeglogNum(expectation));
    }
    return strm;
  }

  ostream &Write(ostream &strm) const {
    WriteType(strm, Value());
    WriteType(strm, (int64)(GetMDExpectations().size()));
    for(MDExpectations::const_iterator it=GetMDExpectations().begin();
        it != GetMDExpectations().end(); ++it){
      WriteType(strm, (int64)(it->first)); // write all the expectations that fire
      // TODO: write sign too
      WriteType(strm, (it->second.lx));
    }
    return strm;
  }

  size_t Hash() const {
    size_t seed = 0;
    boost::hash_combine(seed, Value());
    const MDExpectations& e = GetMDExpectations();
    for(MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it){
      boost::hash_combine(seed, it->second.lx);
      boost::hash_combine(seed, it->second.sign_x);
    }
    return seed;
  }

 private:

  MDExpectations* expectations_;

 protected:

  static const MDExpectations& getEmptyStaticMDExpectations(){
    static const MDExpectations empty;
    return empty;
  }

};

inline MDExpectationWeight Plus(const MDExpectationWeight &w1, const MDExpectationWeight &w2) {
  using namespace fstrain::core;
  double f1 = w1.Value(), f2 = w2.Value(), f3 = 0.0f;
  if (f1 == fstrain::core::kPosInfinity)
    f3 = f2;
  else if (f2 == fstrain::core::kPosInfinity)
    f3 = f1;
  else if (f1 > f2)
    f3 = f2 - LogExp(f1 - f2);
  else
    f3 = f1 - LogExp(f2 - f1);
  MDExpectationWeight w3(f3);
  fstrain::util::AddMaps(w1.GetMDExpectations().begin(), w1.GetMDExpectations().end(),
                         w2.GetMDExpectations().begin(), w2.GetMDExpectations().end(),
                         NeglogPlus,
                         &(w3.GetMDExpectations()));
  return w3;
}

struct NeglogTimesFct {
  typedef fstrain::core::NeglogNum NeglogNum;
  inline NeglogNum operator()(const NeglogNum& a, const NeglogNum& b) const {
    return NeglogTimes(a, b);
  }
};

inline MDExpectationWeight Times(const MDExpectationWeight &w1, const MDExpectationWeight &w2) {
  using namespace fstrain::core;
  double f1 = w1.Value();
  // speedup
  if (f1 == 0.0 && w1.GetMDExpectations().size() == 0) {
    return w2;
  }
  double f2 = w2.Value();
  if (f2 == 0.0 && w2.GetMDExpectations().size() == 0) {
    return w1;
  }
  double f3;
  if (f1 == fstrain::core::kPosInfinity)
    f3 = f1;
  else if (f2 == fstrain::core::kPosInfinity)
    f3 = f2;
  else
    f3 = f1 + f2;
  MDExpectationWeight w3(f3);
  // lazy multiplication with scalar:
  typedef fstrain::util::MultipliedMap<MDExpectationWeight::MDExpectations, NeglogTimesFct> MyMultipliedMap;
  MyMultipliedMap v1_times_p2(w1.GetMDExpectations(), NeglogTimesFct(), NeglogNum(w2.Value()));
  MyMultipliedMap v2_times_p1(w2.GetMDExpectations(), NeglogTimesFct(), NeglogNum(w1.Value()));
  fstrain::util::AddMaps(v1_times_p2.begin(), v1_times_p2.end(),
                         v2_times_p1.begin(), v2_times_p1.end(),
                         NeglogPlus,
                         &(w3.GetMDExpectations()));
  return w3;
}

inline MDExpectationWeight OneOver(const MDExpectationWeight& w){
  using namespace fstrain::core;
  NeglogNum one_over_p = NeglogDivide(NeglogNum(0.0),
                                      NeglogNum(w.Value()));
  NeglogNum minus_one_over_p = NeglogNum(one_over_p.lx, false);
  MDExpectationWeight result(one_over_p.lx);
  const MDExpectations& in = w.GetMDExpectations();
  MDExpectations& out = result.GetMDExpectations();
  for(MDExpectations::const_iterator it = in.begin(); it != in.end(); ++it){
    out.insert(it->first, NeglogTimes(NeglogTimes(minus_one_over_p, it->second),
                                      one_over_p));
  }
  return result;
}

inline MDExpectationWeight Divide(const MDExpectationWeight &w1,
                                  const MDExpectationWeight &w2,
                                  DivideType typ = DIVIDE_ANY) {
  return Times(w1, OneOver(w2));
}

inline ostream &operator<<(ostream &strm, const MDExpectationWeight &w) {
  if (w.Value() == fstrain::core::kPosInfinity)
    strm << "Infinity";
  else if (w.Value() == fstrain::core::kNegInfinity)
    strm << "-Infinity";
  else if (w.Value() != w.Value())   // Fails for NaN
    strm << "BadFloat";
  else
    strm << w.Value();
  w.GetMDExpectations().print(strm);
  return strm;
}

inline istream &operator>>(istream &strm, MDExpectationWeight &w) {
  using fstrain::core::MDExpectations;
  std::string s;
  strm >> s;
  std::string::size_type expectationsStart = s.find('[');
  std::string dStr;
  std::string expectationsStr;
  if(expectationsStart == std::string::npos){
    dStr = s;
    double d = fstrain::core::stringToDouble(dStr);
    w = MDExpectationWeight(d);
  }
  else{
    if(s.find(']') == std::string::npos){
      throw std::runtime_error("Format error in expectations");
    }
    dStr = s.substr(0, expectationsStart);
    expectationsStr = s.substr(expectationsStart + 1, s.length() - expectationsStart - 2);
    double d = fstrain::core::stringToDouble(dStr);
    w = MDExpectationWeight(d);
    w.GetMDExpectations().insert(expectationsStr);
  }
  return strm;
}

inline bool ApproxEqual(const MDExpectationWeight& w1,
                        const MDExpectationWeight& w2,
                        float delta = fst::kDelta) {
  return fstrain::core::ApproxEqual(w1.Value(), w2.Value(), delta)
      && fstrain::core::ApproxEqual(w1.GetMDExpectations(), w2.GetMDExpectations(), delta);
}

template <>
struct WeightConvert<Log64Weight, MDExpectationWeight> {
  MDExpectationWeight operator()(Log64Weight w) const { return w.Value(); }
};

template <>
struct WeightConvert<MDExpectationWeight, Log64Weight> {
  Log64Weight operator()(MDExpectationWeight w) const { return w.Value(); }
};

template <>
struct WeightConvert<LogWeight, MDExpectationWeight> {
  MDExpectationWeight operator()(LogWeight w) const { return w.Value(); }
};

template <>
struct WeightConvert<MDExpectationWeight, LogWeight> {
  LogWeight operator()(MDExpectationWeight w) const { return w.Value(); }
};

template <>
struct WeightConvert<TropicalWeight, MDExpectationWeight> {
  MDExpectationWeight operator()(TropicalWeight w) const { return w.Value(); }
};

template <>
struct WeightConvert<MDExpectationWeight, TropicalWeight> {
  TropicalWeight operator()(MDExpectationWeight w) const { return w.Value(); }
};

}  // end namespace fst


#endif
