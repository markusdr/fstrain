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

#ifndef FSTRAIN_UTIL_ADD_MAPS_H_
#define FSTRAIN_UTIL_ADD_MAPS_H_

namespace fstrain { namespace util {

/**
 * @brief Adds two maps by building the union of their keys and adding
 * up their values if they are present in both maps.
 *
 * @param begin1 The begin of first map.
 * @param end1 The end of first map.
 * @param begin2 The begin of second map.
 * @param end2 The end of second map.
 * @param Resulting map
 */
template<typename InputIterator1, typename InputIterator2,
         typename AddFct,
         typename OutputMap>
void AddMaps(InputIterator1 begin1, InputIterator1 end1,
             InputIterator2 begin2, InputIterator2 end2,
             AddFct add,
             OutputMap* result) {
  while (begin1 != end1 && begin2 != end2) {
    if (begin1->first < begin2->first) {
      (*result)[begin1->first] = begin1->second;
      ++begin1;
    }
    else if (begin2->first < begin1->first) {
      (*result)[begin2->first] = begin2->second;
      ++begin2;
    }
    else {
      assert(begin1->first == begin2->first);
      (*result)[begin1->first] = add(begin1->second, begin2->second);
      ++begin1;
      ++begin2;
    }
  }
  result->insert(begin1, end1);
  result->insert(begin2, end2);
}

} } // end namespaces

#endif
