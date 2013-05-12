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
#ifndef FSTRAIN_CORE_NEG_LOG_OF_SIGNED_NUM_H
#define FSTRAIN_CORE_NEG_LOG_OF_SIGNED_NUM_H

#include <climits>
#include <cmath>
#include <cassert>
#include "fstrain/core/util.h"

namespace fstrain { namespace core {

/**
 * @brief Represents a positive or negative number x in log space.
 */
struct NeglogNum {
  double lx;        // -log(abs(x))
  bool sign_x;      // sign of x (true means plus)

  NeglogNum() 
      // : lx(std::numeric_limits<double>::infinity()), 
      : lx(kPosInfinity), 
        sign_x(true) {}

  /**
   * @brief Constructor.
   *
   * @param lx_ The negated log of |x| if you want to represent x,
   * e.g. -log(abs(-1/2)) for x=-1/2
   *
   * @param sign_x false if the number you want to represent is
   * negative.
   */
  NeglogNum(double lx_, bool sign_x_ = true) 
      : lx(lx_), sign_x(sign_x_) {}

  static const NeglogNum Zero() {
    return NeglogNum(std::numeric_limits<double>::infinity());
  }

  static const NeglogNum One() {
    return NeglogNum(0.0F);
  }
};

//NeglogNum NeglogOfPosNum(double l) { return NeglogNum(l); }
//NeglogNum NeglogOfNegNum(double l) { return NeglogNum(l, false); }

inline bool operator==(const NeglogNum& a,
		       const NeglogNum& b) {
  return a.lx == b.lx && a.sign_x == b.sign_x;
} 
inline bool operator!=(const NeglogNum& a,
		       const NeglogNum& b) {
  return !(a==b);
} 

/**
 * @return The original, encoded number, e.g. -0.5 from
 * NeglogNum(-log(0.5), false).
 */
inline double GetOrigNum(const NeglogNum& n) {
  return (n.sign_x ? 1.0 : -1.0) * exp(-n.lx);
}

inline std::ostream& operator<<(std::ostream& out, const NeglogNum& n) {  
  out << (n.sign_x || n.lx == 0 ? "" : "!") // the '!' marks that a negative num is represented
      << n.lx;
  return out;
}

inline NeglogNum NeglogPlus_(const NeglogNum& a, 
			     const NeglogNum& b) {
  assert(a.lx <= b.lx);
  double factor = a.sign_x == b.sign_x ? 1.0 : -1.0;
  return NeglogNum(a.lx - log1p(factor * exp(a.lx - b.lx)),
		   a.sign_x);
}

inline NeglogNum NeglogPlus(const NeglogNum& a, 
			    const NeglogNum& b) {
  if(a.lx == std::numeric_limits<double>::infinity()) {
    return b;
  }
  if(b.lx == std::numeric_limits<double>::infinity()) {
    return a;
  }
  if(a.lx <= b.lx) {
    return NeglogPlus_(a, b);
  }
  else {
    return NeglogPlus_(b, a);    
  }
}
inline NeglogNum NeglogTimes(const NeglogNum& a, 
			     const NeglogNum& b) {
  if(a.lx == std::numeric_limits<double>::infinity()){
    return a;
  }
  if(b.lx == std::numeric_limits<double>::infinity()){
    return b;
  }
  return NeglogNum(a.lx + b.lx,
		   a.sign_x == b.sign_x);    
}

inline NeglogNum NeglogDivide(const NeglogNum& a, 
			      const NeglogNum& b) {
  if (b.lx == std::numeric_limits<double>::infinity()) {
    // return NeglogNum(std::numeric_limits<double>::quiet_NaN());
    return NeglogNum(-std::numeric_limits<double>::infinity()); // x/0.0=inf
  }
  else if (a.lx == std::numeric_limits<double>::infinity()) {
    return a;
  }
  else {
    return NeglogNum(a.lx - b.lx,
		     a.sign_x == b.sign_x);    
  }
}

inline NeglogNum NeglogQuantize(const NeglogNum& n, double delta) {
  if (n.lx == -std::numeric_limits<double>::infinity() 
      || n.lx == std::numeric_limits<double>::infinity() 
      || n.lx != n.lx) {
    return n;
  }
  else {
    return NeglogNum(floor(n.lx / delta + 0.5F) * delta,
		     n.sign_x); 
  }
}

} } // end namespaces

#endif
