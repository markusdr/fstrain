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
 * $Id: data.h 5947 2009-06-23 18:07:28Z markus $
 *
 * @todo Make a separate class, RandomAccessData
 * @brief Represents data input/output pairs
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_UTIL_DATA_H_
#define FSTRAIN_UTIL_DATA_H_

#include <iostream>
#include <vector>
#include <string>

namespace fstrain { namespace util {

  class Data {

  private:

    typedef std::vector< std::pair<std::string,std::string> > Container;
    Container data_;

    void init(const char* filename);
    void init_from_stream(std::istream& in);

  public:

    typedef Container::const_iterator const_iterator;
    typedef Container::size_type size_type;

    Data() {}

    Data(const std::string& filename) {
      init(filename.c_str());
    }

    Data(const char* filename) {
      init(filename);
    }

    Data(std::istream& in) {
      init_from_stream(in);
    }

    const_iterator begin () const {
      return data_.begin();
    }

    const_iterator end () const {
      return data_.end();
    }

    void push_back(const std::string& in, const std::string& out);

    size_type size() const {
      return data_.size();
    }

    const std::pair<std::string, std::string>& operator[](int index) const {
      return data_[index];
    }

  };

} } // end namespaces

#endif

