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

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fstrain/create/get-features.h"

BOOST_AUTO_TEST_SUITE(features)

BOOST_AUTO_TEST_CASE( GetFeatures_1 ) {
  using namespace fstrain::create;
  std::string result;
  int n = GetNumPartsAndPrepareString("S|S a|x-c1 b|y-c2", &result);
  BOOST_CHECK_EQUAL(result, "S|S-?? a|x-c1 b|y-c2");
  BOOST_CHECK_EQUAL(n, 1);
}

BOOST_AUTO_TEST_CASE( GetFeatures_2 ) {
  using namespace fstrain::create;
  std::string result;
  int n = GetNumPartsAndPrepareString("a|-", &result);
  BOOST_CHECK_EQUAL(result, "a|-");
  BOOST_CHECK_EQUAL(n, 0);
}

BOOST_AUTO_TEST_CASE( GetFeatures_3 ) {
  using namespace fstrain::create;
  std::string result;
  int n = GetNumPartsAndPrepareString("S|S a|x-c1-g3", &result);
  BOOST_CHECK_EQUAL(result, "S|S-??-?? a|x-c1-g3");
  BOOST_CHECK_EQUAL(n, 2);
}

BOOST_AUTO_TEST_CASE( GetFeatures_4 ) {
  using namespace fstrain::create;
  std::string result;
  int n = GetNumPartsAndPrepareString("S|S a|x", &result);
  BOOST_CHECK_EQUAL(result, "S|S a|x");
  BOOST_CHECK_EQUAL(n, 0);
}

BOOST_AUTO_TEST_SUITE_END()


