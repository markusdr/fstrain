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
 * Singleton displays the current usage of virtual memory.
 *
 * Usage: MemoryInfo::instance().getSize();
 *        MemoryInfo::instance().getSizeInMB();
 *
 * This is useful for debug output and for deciding when to call gc().
 *
 * @author Markus Dreyer
 */

#ifndef FSTRAIN_UTIL_MEMORYINFO_H_
#define FSTRAIN_UTIL_MEMORYINFO_H_

#include <iostream>
#include <fstream>

namespace fstrain { namespace util {

class MemoryInfo{

public:

    // Singleton
    MemoryInfo();
    MemoryInfo(const MemoryInfo&);
    static MemoryInfo& instance();

    std::size_t getSize();
    double getSizeInMB();

private:

    char memoryFilename[64];
    std::ifstream memoryFileStream;
    std::string getColumn(const std::string& s, unsigned columnNumber);

};

} } // end namespace fstrain::util

#endif // _MEMORYINFO_H_
