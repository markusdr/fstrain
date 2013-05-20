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
#ifndef FSTR_CREATE_DBG_H
#define FSTR_CREATE_DBG_H

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

#ifdef FSTR_CREATE_DBG_WITH_LINEINFO
#define FSTR_CREATE_DBG_LINE_INFO << __FILE__ << ":" << __LINE__ << ":"
#else
#define FSTR_CREATE_DBG_LINE_INFO
#endif

#ifndef NDEBUG
#define FSTR_CREATE_DBG_MSG(n,x) { char* level_xyz = getenv("FSTR_CREATE_DBG_LEVEL"); int level_xyz_int =level_xyz ? atoi(level_xyz) : 0; if(level_xyz_int>=n){std::stringstream ss123; ss123 << x; std::cerr FSTR_CREATE_DBG_LINE_INFO << ss123.str();} }
#define FSTR_CREATE_DBG_EXEC(n,x) { char* level_xyz = getenv("FSTR_CREATE_DBG_LEVEL"); int level_xyz_int = level_xyz ? atoi(level_xyz) : 0; if(level_xyz_int>=n){x;} }
#else
#define FSTR_CREATE_DBG_MSG(n,x)
#define FSTR_CREATE_DBG_EXEC(n,x)
#endif

#define FSTR_CREATE_ERROR(x){std::stringstream ss123; ss123 << "fstrain train error: " << x; std::cerr << __FILE__ << ":" << __LINE__ << ":" << ss123.str() << std::endl; exit(EXIT_FAILURE);}

#define FSTR_CREATE_EXCEPTION(x){std::stringstream ss123; ss123 << __FILE__ << ":" << __LINE__ << ":" << x << std::endl; throw std::runtime_error(ss123.str());}

#endif
