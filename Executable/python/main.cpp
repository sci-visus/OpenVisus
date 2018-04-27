#include <Python.h>
#include <pydebug.h>

#include <locale.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
#if PY_MAJOR_VERSION >= 3

int main(int argn, const char* argv[])
{
  //correct args
  std::vector<wchar_t*> Argv;
  char *oldloc = _PyMem_RawStrdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "");

  //if (_Py_LegacyLocaleDetected())
  //  _Py_CoerceLegacyLocale();

  #if PY_MINOR_VERSION<=4
  #define Py_DecodeLocale _Py_char2wchar
  #endif

  for (int i = 0; i < argn; i++)
    Argv.push_back(Py_DecodeLocale(argv[i], NULL));
  Argv.push_back(NULL);

  setlocale(LC_ALL, oldloc);
  PyMem_RawFree(oldloc);

  auto ToFree = Argv;
  int res = Py_Main((int)Argv.size() - 1, &Argv[0]);

  for (auto it : ToFree) {
    if (it)
      PyMem_RawFree(it);
  }

  return res;
}

#else

int main(int argn, const char* argv[])
{
  return Py_Main(argn, (char**)argv);
}

#endif
