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

#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include "fstrain/util/memory-info.h"
#include "fstrain/util/options.h"
#include "fstrain/util/debug.h"
#include <boost/lexical_cast.hpp>

namespace fstrain { namespace util {

MemoryInfo::MemoryInfo(const MemoryInfo&){
    throw std::runtime_error("Not implemented. Please use MemoryInfo::instance().getSize(); (or getSizeInMB())");
}

MemoryInfo::MemoryInfo(){
    snprintf(memoryFilename, 63, "/proc/%d/stat", getpid());
}

MemoryInfo& MemoryInfo::instance(){
	static MemoryInfo the_instance;
	return the_instance;
}

std::size_t MemoryInfo::getSize(){
  // unsigned long long memoryUsage;
    memoryFileStream.open(memoryFilename, std::ifstream::in);
    std::string line;
    std::getline(memoryFileStream, line);
    memoryFileStream.close();
    const std::string val0 = getColumn(line, 22); // virtual memory size column
    std::size_t val = boost::lexical_cast<std::size_t>(val0);
    if(fstrain::util::options.has("memory-limit-bytes")
       && val > fstrain::util::options.get<double>("memory-limit-bytes")) {
      std::cerr << "Error: Memory limit exceeded (" << val << " bytes)" << std::endl;
      FSTR_UTIL_EXCEPTION("Memory limit exceeded");
    }
    return val;
}

double MemoryInfo::getSizeInMB(){
  return getSize() / 1048576.0;
}

/**
 * Returns the specified column of a string.
 * (columns are blank-separated, counting from 0)
 */
std::string MemoryInfo::getColumn(const std::string& s, unsigned columnNumber){
  std::string::size_type start = 0;
  while(columnNumber-- > 0){
    start = s.find(' ', start) + 1;
  }
  return s.substr(start, s.find(' ', start + 1) - start);
}

} } // end namespace fstrain::util
