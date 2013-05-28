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
// Author: jrs026@gmail.com (Jason Smith)
//
#ifndef FSTRAIN_CREATE_WF_UTIL_H_
#define FSTRAIN_CREATE_WF_UTIL_H_

#include <sstream>
#include "fst/vector-fst.h"
#include "fst/symbol-table.h"
// #include "src/bin/print-main.h"

namespace fstrain { namespace create {

namespace wfutil
{

	template <class T>
	std::string toString (const T& t)
	{
		std::stringstream ss;
		ss << t;
		return ss.str();
	}

	// Extracts a latent annotation value from a symbol
	// getLA("-|z-c2-g3", "-g") == 3
	int getLA(const string& symbol, const string& la);

}

} } // end namespaces fstrain::create

#endif
