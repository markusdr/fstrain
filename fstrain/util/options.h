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
#ifndef FSTRAIN_UTIL_OPTIONS_H
#define FSTRAIN_UTIL_OPTIONS_H

#include <string>
#include <map>
#include <boost/any.hpp> // TODO: would rather not expose this in header

namespace fstrain { namespace util {

class Options : public std::map<std::string, boost::any> {

 public:

  Options();

  template<class T>
  T get(const std::string& key)  {
    return boost::any_cast<T>((*this)[key]);
  }

  bool has(const std::string& key) const;
};

extern Options options;

} } // end namespaces

#endif
