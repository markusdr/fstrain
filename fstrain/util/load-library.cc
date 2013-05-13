#ifndef FSTRAIN_UTIL_LOAD_LIBRARY_H
#define FSTRAIN_UTIL_LOAD_LIBRARY_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE my_lib_t;
#else
#include <dlfcn.h>
typedef void* my_lib_t;
#endif

namespace fstrain { namespace util {

my_lib_t LoadLib(const char* szLib) {
# ifdef _WIN32
  return ::LoadLibraryA(szLib);
# else //_WIN32
  return ::dlopen(szLib, RTLD_LAZY);
# endif //_WIN32
}

void UnloadLib(my_lib_t hLib) {
# ifdef _WIN32
  ::FreeLibrary(hLib);
# else //_WIN32
  ::dlclose(hLib);
# endif //_WIN32
}

void* LoadProc(my_lib_t hLib, const char* szProc) {
# ifdef _WIN32
  return ::GetProcAddress(hLib, szProc);
# else //_WIN32
  return ::dlsym(hLib, szProc);
# endif //_WIN32
}

} } // end namespaces

#endif
