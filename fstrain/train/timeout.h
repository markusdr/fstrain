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
#ifndef FSTRAIN_TRAIN_TIMEOUT_H
#define FSTRAIN_TRAIN_TIMEOUT_H

#include <sys/time.h>

namespace fstrain { namespace train {

/**
 * @brief Function object that can signal a timeout.
 */
struct ITimeout {
  virtual ~ITimeout() {}
  virtual bool operator()() = 0;
};

/**
 * @brief Signals a timeout after a previously specified time.
 */
class Timeout : public ITimeout {

 public:

  /**
   * @brief Constructor.
   * @param time_limit The time limit in milliseconds.
   *                   Value -1 means no time limit.
   */
  Timeout(long time_limit)
      : time_limit_(time_limit) {
    gettimeofday(&start_time, NULL);
  }

  bool operator()() {
    if (time_limit_ != -1) {
      long elapsed_time = GetElapsedTime();
      // std::cerr << elapsed_time << "ms < " << time_limit_ << "ms" << std::endl;
      if (elapsed_time >= time_limit_) {
	std::cerr << "Timeout." << std::endl;
        return true;
      }
    }
    return false;
  }

  long GetElapsedTime() const {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    return (curr_time.tv_sec - start_time.tv_sec) * 1000
        + (curr_time.tv_usec - start_time.tv_usec) / 1000;
  }

 private:
  struct timeval start_time;
  long time_limit_;
};

} } // end namespace fstrain::train

#endif
