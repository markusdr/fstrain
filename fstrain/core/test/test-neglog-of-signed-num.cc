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

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>
#include "fstrain/core/neg-log-of-signed-num.h"

void Test(bool b) {
  if(b){
    std::cout << "OK" << std::endl;
  }
  else {
    throw std::runtime_error("FAIL");
  }
}

bool ApproxEqual(double a, double b, double delta = 1e-6) {
  return
      fabs(a-b) < delta
      || (a==std::numeric_limits<double>::infinity() && b==std::numeric_limits<double>::infinity())
      || (a==-std::numeric_limits<double>::infinity() && b==-std::numeric_limits<double>::infinity());
}

void TestDivide(double a, double b) {
  using namespace fstrain::core;
  std::cout << "TestDivide(" << a << ", " << b << "): ";
  NeglogNum la = NeglogNum(-log(fabs(a)), a < 0);
  NeglogNum lb = NeglogNum(-log(fabs(b)), b < 0);
  NeglogNum result = NeglogDivide(la, lb);
  double factor = result.sign_x ? 1.0 : -1.0;
  double exp_result = factor * exp(-result.lx);
  Test(ApproxEqual(exp_result, a/b));
  if(exp_result != 0 && a/b != 0){
    Test(!ApproxEqual(exp_result, -a/b)); // test the sign too
  }
}

void TestTimes(double a, double b) {
  using namespace fstrain::core;
  std::cout << "TestTimes(" << a << ", " << b << "): ";
  NeglogNum la = NeglogNum(-log(fabs(a)), a < 0);
  NeglogNum lb = NeglogNum(-log(fabs(b)), b < 0);
  NeglogNum result = NeglogTimes(la, lb);
  double factor = result.sign_x ? 1.0 : -1.0;
  double exp_result = factor * exp(-result.lx);
  Test(ApproxEqual(exp_result, a*b));
  if(exp_result != 0 && a*b != 0){
    Test(!ApproxEqual(exp_result, -a*b)); // test the sign too
  }
}

void TestPlus(double a, double b) {
  using namespace fstrain::core;
  std::cout << "TestPlus(" << a << ", " << b << "): ";
  NeglogNum la = NeglogNum(-log(fabs(a)), a >= 0);
  NeglogNum lb = NeglogNum(-log(fabs(b)), b >= 0);
  NeglogNum result = NeglogPlus(la, lb);
  double factor = result.sign_x ? 1.0 : -1.0;
  double exp_result = factor * exp(-result.lx);
  std::cout << exp_result << " == " << a+b << std::endl;
  std::cout << "result=" << result << std::endl;
  Test(ApproxEqual(exp_result, a+b));
  if(exp_result != 0 && a+b != 0){
    Test(!ApproxEqual(exp_result, -(a+b))); // test the sign too
  }
}

int main(int argc, char** argv){
  try{
    TestDivide(1, 2);
    TestDivide(-1, 2);
    TestDivide(-3, 10);
    TestDivide(3, -10);
    TestDivide(-3.11, -10.5);
    TestDivide(1/3.0, -1/3.0);
    TestDivide(0, -1/3.0);
    TestDivide(1/3.0, 1e-5);
    TestDivide(1/3.0, 0.0);

    TestTimes(1, 2);
    TestTimes(-1, 2);
    TestTimes(-3, 10);
    TestTimes(3, -10);
    TestTimes(-3.11, -10.5);
    TestTimes(1/3.0, -1/3.0);
    TestTimes(0, -1/3.0);
    TestTimes(1/3.0, 1e-5);
    TestTimes(1/3.0, 0.0);

    TestPlus(1, 2);
    TestPlus(-1, 2);
    TestPlus(-3, 10);
    TestPlus(3, -10);
    TestPlus(-3.11, -10.5);
    TestPlus(1/3.0, -1/3.0);
    TestPlus(-1/3.0, 1/3.0);
    TestPlus(0, -1/3.0);
    TestPlus(1/3.0, 1e-5);
    TestPlus(1/3.0, 0.0);

  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
