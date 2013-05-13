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
 * $Id: util.h 6197 2009-08-19 23:40:07Z markus $
 *
 * @brief Utility functions
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_CORE_UTIL_H
#define FSTRAIN_CORE_UTIL_H

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#include <cstring> // strcmp
#include <limits>
#include "fstrain/core/debug.h"

namespace fstrain { namespace core {

static const double kPosInfinity = std::numeric_limits<double>::infinity();
static const double kNegInfinity = -kPosInfinity;

// TODO: use StrToInt64 (see text-io.h)
unsigned long stringToUnsignedLong(const std::string& s);

double stringToDouble(const std::string& s);

static inline double LogExp(double x) { return log1p(exp(-x)); }

/**
 * the logplus from OpenFst float-weight.h: -log(e^-x + e^-y)
 *
 */
static inline double logPlus (const double &f1, const double &f2) {
  if (f1 == kPosInfinity)
    return f2;
  else if (f2 == kPosInfinity)
    return f1;
  else if (f1 > f2)
    return f2 - LogExp(f1 - f2);
  else
    return f1 - LogExp(f2 - f1);
}

static inline double mymax2(double x, double y)
{
#ifdef IEEE_754
  if (ISNAN(x) || ISNAN(y))
    return x + y;
#endif
  return (x < y) ? y : x;
}

/**
 * standard logplus defined as log(e^x + e^y)
 */
static inline double log_add(double x, double y) {
  if(x == kNegInfinity)
    return y;
  if(y == kNegInfinity)
    return x;
  return mymax2(x, y) + log1p(exp( -fabs(x - y) ));
}

static inline double log_subtract(double x, double y) {
  if(x < y)
    FSTR_CORE_EXCEPTION("Error: log of negative number");
  if(y == kNegInfinity)
    return x;
  return x + log1p( -exp(y - x) );
}

inline int round(double x){
  return int(std::floor(x < 0 ? x - 0.5 : x + 0.5));
}

/**
 * @brief Implements a cooling schedule. Half of initValue is
 * reached at lastUpdate.
 * @param currTime Current time step (use 0 on first call)
 * @param targetTime Last time step (e.g. 2 if you want to call it with 0, 1, 2)
 * @param searchTime 1.0=pretty linear, 5.0=search long, then converge fast
 * @param linear simple linear function, searchTime ignored.
 */
inline double decreasingSchedule(int currTime, int targetTime,
                                 double initVal,
                                 double searchTime = 1.0, bool linear = false){
  const double targetTime0 = targetTime + 10e-32;
  if(linear){
    const double targetVal = initVal / 2.0;
    return initVal - currTime * (initVal - targetVal) / targetTime0;
  }
  else{
    return initVal / (1.0 + pow(currTime / targetTime0, searchTime));
  }
}

/**
 * @brief Another cooling schedule (exponential).
 * @param i Current time step
 * @param V0 Initial value
 * @param VN Value at time step N
 * @param N Last time step.
 */
inline double decreasingSchedule2(double i, double V0, double VN, double N){
  if(VN == 0){
    VN = 10e-15;
  }
  return V0 * pow(VN / V0, i / N);
}

/**
 * @brief Another cooling schedule (linear).
 * @param i Current time step
 * @param V0 Initial value
 * @param VN Value at time step N
 * @param N Last time step.
 */
inline double decreasingSchedule3(double i, double V0, double VN, double N){
  return V0 - i * (V0 - VN) / N;
}

void tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters = " ",
              bool removeLeadingBlanks = true);

/**
 * General converter w/o checks
 */
template <class out_type, class in_value>
out_type convert(const in_value & t){
  std::stringstream stream;
  stream << t;
  out_type result;
  stream >> result;
  return result;
}

template<class T>
void writeVectorToFile(const std::vector<T>& vec, const std::string& filename){
  std::ofstream out(filename.c_str());
  if(!out.is_open()){
    FSTR_CORE_EXCEPTION("Could not write file " << filename);
  }
  for(typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it){
    out << *it << std::endl;
  }
}

struct strCmp {
  bool operator()( const char* s1, const char* s2 ) const {
    return strcmp( s1, s2 ) < 0;
  }
};

inline std::string trim(std::string& s,const std::string& drop = " \t")
{
  std::string r=s.erase(s.find_last_not_of(drop)+1);
  return r.erase(0,r.find_first_not_of(drop));
}

} } // end namespace fstrain/core

#endif
