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

#include "fstrain/create/features/latent-annotation-features.h"

void PrintFeatures(const std::string& s) {
  std::cerr << s << std::endl;
  using fstrain::create::features::LatentAnnotationFeatures;
  LatentAnnotationFeatures f(s);
  for(LatentAnnotationFeatures::const_iterator it = f.begin(); 
      it != f.end(); ++it) {
    std::cerr << "F: '" << *it << "'" << std::endl;
  }
  std::cerr << std::endl;
}

BOOST_AUTO_TEST_SUITE(la);

BOOST_AUTO_TEST_CASE( MyTestCase ) {   

  const std::string s = "a|x-c1-g3 b|--c2-g1 b|y-c2-g3";
  PrintFeatures(s);

  const std::string s2 = "S|S --c2-g1 b|y-c2-g3 E|E";
  PrintFeatures(s2);

  const std::string s3 = "-|x a|- --c2";
  PrintFeatures(s3);
  
}

BOOST_AUTO_TEST_SUITE_END()


  
  
  
  
