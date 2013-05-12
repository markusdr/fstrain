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
#include <sstream>
#include "fstrain/create/wfutil.h"

namespace fstrain { namespace create {

namespace wfutil
{

	int getLA(const string& symbol, const string& la)
	{
		return atoi(symbol.substr(
				symbol.find(la, 3) + la.length(),
				symbol.find("-", symbol.find(la, 3) + la.length()) - 
				symbol.find(la, 3) + la.length()
			).c_str());
	}

}

} } // end namespaces fstrain::create
