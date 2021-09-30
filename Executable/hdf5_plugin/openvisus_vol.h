#ifndef _OPENVISUS_VOL_H__
#define _OPENVISUS_VOL_H__

/* Public headers needed by this file */
#include "H5VLpublic.h"        /* Virtual Object Layer                 */


#if WIN32
#  define OPENVISUS_H5F_PLUGIN_SHARED_EXPORT __declspec (dllexport)
#else 
#  define OPENVISSU_H5F_PLUGIN_SHARED_EXPORT __attribute__ ((visibility("default")))
#endif 

// The HDF5 library _must_ find routines with these names and signatures
extern "C" OPENVISUS_H5F_PLUGIN_SHARED_EXPORT H5PL_type_t H5PLget_plugin_type(void);
extern "C" OPENVISUS_H5F_PLUGIN_SHARED_EXPORT const void* H5PLget_plugin_info(void);


#endif //_OPENVISUS_VOL_H__
