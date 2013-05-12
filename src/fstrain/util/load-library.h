#ifndef FSTRAIN_UTIL_LOAD_LIBRARY_H
#define FSTRAIN_UTIL_LOAD_LIBRARY_H

namespace fstrain { namespace util {

#ifdef _WIN32
#include <windows.h>
typedef HANDLE my_lib_t;
#else
#include <dlfcn.h>
typedef void* my_lib_t;
#endif

my_lib_t LoadLib(const char* szLib);

void UnloadLib(my_lib_t hLib);

void* LoadProc(my_lib_t hLib, const char* szProcl);

} } // end namespaces

#endif
