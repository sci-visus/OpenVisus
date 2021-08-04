#ifndef VISUS_MINIMAL_HEADER_H__
#define VISUS_MINIMAL_HEADER_H__

/*
NOTE: this file should not use any OpenVisus header file
It is used as a simplified wrapper layer for old C++98 compilers
*/
#include <string>

// I'm 'using' the library with an old C++98 compiler
#ifndef VISUS_DB_API

  #if WIN32
    #define VISUS_SHARED_EXPORT __declspec (dllexport)
    #define VISUS_SHARED_IMPORT __declspec (dllimport)
  #else 
    #define VISUS_SHARED_EXPORT __attribute__ ((visibility("default")))
    #define VISUS_SHARED_IMPORT 
  #endif 

  #if VISUS_STATIC_LIB
    #define VISUS_DB_API
  #else
    #if VISUS_BUILDING_VISUSDB
    #define VISUS_DB_API VISUS_SHARED_EXPORT
    #else
    #define VISUS_DB_API VISUS_SHARED_IMPORT
    #endif
  #endif
#endif

namespace Visus {


/// <summary>
/// /////////////////////////////////////////////////////////////////
/// </summary>
class VISUS_DB_API MinimalAccess
{
public:

  void* pimpl;

  //constructor
  MinimalAccess(void* pimpl_) : pimpl(pimpl_) {
  }

  //destructor
  ~MinimalAccess();
};

/// <summary>
/// /////////////////////////////////////////////////////////////////
/// </summary>
class VISUS_DB_API MinimalDataset
{
public:

  void* pimpl;

  //Create
  static void Create(std::string idx_filename, std::string dtype, int N_x, int N_y, int N_z);

  //constructor (must be destroyed by the caller)
  static MinimalDataset* Load(std::string idx_filename);

  //constructor
  MinimalDataset(void* pimpl_) : pimpl(pimpl_) {
  }

  //destructor
  ~MinimalDataset();

  //createAccess (must be destroyed by the caller)
  MinimalAccess* createAccess();

  //writeData
  void writeData(MinimalAccess* access, int x1, int y1, int z1, int x2, int y2, int z2, Uint8* buffer, int buffer_size);

  //readData
  void readData(MinimalAccess* access, int x1, int y1, int z1, int x2, int y2, int z2, Uint8* buffer, int buffer_size);

};

VISUS_DB_API void InitMinimalModule();
VISUS_DB_API void ShutdownMinimalModule();

} //namespace Visus

#endif //VISUS_MINIMAL_HEADER_H__