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
#ifndef FSTRAIN_UTIL_TIMER_H
#define FSTRAIN_UTIL_TIMER_H
#include <sys/time.h>
#include <iostream>

namespace fstrain { namespace util {

class Timer{
 public:

  Timer();
  ~Timer();

  void start();
  void stop();
  double get_elapsed_time_millis();
  double get_elapsed_time_seconds();

 private:
  struct timeval startTime;
  struct timeval stopTime; // start and stop times structures
  long elapsedTime;
  bool running;

};

} } // end namespaces

#endif // TIMER_H
