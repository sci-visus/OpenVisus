#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hdf5.h>

#include "openvisus_vol.h"

#include <Visus/Kernel.h>
#include <Visus/Db.h>
#include <Visus/Path.h>
#include <Visus/StringUtils.h>
#include <Visus/Utils.h>
#include <Visus/DType.h>
#include <Visus/Dataset.h>
#include <Visus/IdxDataset.h>
#include <Visus/File.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

#define ENABLE_LOGGING 1

using namespace Visus;

#define USE_OPENVISUS_DATASET 1

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1800)
#define va_copy(D,S)      ((D) = (S))
#endif

typedef struct openvisus_vol_info_t 
{
  hid_t under_vol_id;         /* VOL ID for under VOL */
  void* under_vol_info;       /* VOL info for under VOL */
}
openvisus_vol_info_t;

typedef struct openvisus_vol_t 
{
  hid_t under_vol_id;     // ID for underlying VOL connector 
  void* under;      // create the dataset anyway (but not the storage)
  SharedPtr<Dataset>* db;  // Info object for underlying VOL connector
}
openvisus_vol_t;


typedef struct openvisus_vol_wrap_ctx_t 
{
  hid_t under_vol_id;         /* VOL ID for under VOL */
  void* under_wrap_ctx;       /* Object wrapping context for under VOL */
}
openvisus_vol_wrap_ctx_t;

extern const H5VL_class_t openvisus_vol_g;

/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_new_obj
 *
 * Purpose:     Create a new object for an underlying object
 *
 * Return:      Success:    Pointer to the new object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */

static openvisus_vol_t*
openvisus_vol_new_obj(void* under, hid_t under_vol_id, SharedPtr<Dataset> db= SharedPtr<Dataset>())
{
  openvisus_vol_t* ret = (openvisus_vol_t*)calloc(1, sizeof(openvisus_vol_t));
  ret->under_vol_id = under_vol_id;
  ret->under = under;
  ret->db = db? new SharedPtr<Dataset>(db) : nullptr;
  H5Iinc_ref(ret->under_vol_id);
  return ret;
}


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_free_obj
 *
 * Purpose:     Release a object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_free_obj(openvisus_vol_t* obj)
{
  hid_t err_id;
  err_id = H5Eget_current_stack();
  if (obj->db)
  {
    (*obj->db).reset();
    delete obj->db;
  }
  H5Idec_ref(obj->under_vol_id);
  H5Eset_current_stack(err_id);
  free(obj);
  return 0;
}

/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *              operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static bool detach_python = false;

static herr_t
openvisus_vol_init(hid_t vipl_id)
{
  unsigned int m1, m2, m3;
  H5get_libversion(&m1,&m2,&m3);
  PrintInfo("HDF5 runtime version", m1, m2, m3);
  PrintInfo("HDF5 compiled version", H5_VERSION);

  //checkversion
  auto v = StringUtils::split(H5_VERSION, ".");
  VisusReleaseAssert(H5check_version(cint(v[0]), cint(v[1]), 0) == 0);

//#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_init","H5_VERSION", H5_VERSION);
  //#endif

#if USE_OPENVISUS_DATASET

#if VISUS_PYTHON

  if (!Py_IsInitialized())
  {
    detach_python = true;

    static int argn = 1;
    static String program_name = Path(Utils::getCurrentApplicationFile()).toString();
    static const char* argv[] = { program_name.c_str() };

    SetCommandLine(argn, argv);
    EmbeddedPythonInit();
    VisusReleaseAssert(Py_IsInitialized());
  }

  // I need to make sure OpenVisus is imported
  auto acquire_gil = PyGILState_Ensure();
  PyRun_SimpleString("from OpenVisus import *");
  PyGILState_Release(acquire_gil);
#endif

  DbModule::attach();

#endif

  return 0;
} /* end openvisus_vol_init() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *              operations for the connector that release connector-wide
 *              resources (usually created / initialized with the 'init'
 *              callback).
 *
 * Return:      Success:    0
 *              Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_term(void)
{


#if USE_OPENVISUS_DATASET

  DbModule::detach();
  
#if VISUS_PYTHON
  if (detach_python)
  {
    EmbeddedPythonShutdown();
    detach_python = false;
  }
#endif

#endif

  //#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_term");
  //#endif

  return 0;
} 


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns:     Success:    New connector info object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void*
openvisus_vol_info_copy(const void* _info)
{
  const openvisus_vol_info_t* info = (const openvisus_vol_info_t*)_info;
  openvisus_vol_info_t* new_info;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_info_copy");
#endif

  /* Allocate new VOL info struct for the connector */
  new_info = (openvisus_vol_info_t*)calloc(1, sizeof(openvisus_vol_info_t));

  /* Increment reference count on underlying VOL ID, and copy the VOL info */
  new_info->under_vol_id = info->under_vol_id;
  H5Iinc_ref(new_info->under_vol_id);
  if (info->under_vol_info)
    H5VLcopy_connector_info(new_info->under_vol_id, &(new_info->under_vol_info), info->under_vol_info);

  return new_info;
} /* end openvisus_vol_info_copy() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *              following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_info_cmp(int* cmp_value, const void* _info1, const void* _info2)
{
  const openvisus_vol_info_t* info1 = (const openvisus_vol_info_t*)_info1;
  const openvisus_vol_info_t* info2 = (const openvisus_vol_info_t*)_info2;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_info_cmp");
#endif

  /* Sanity checks */
  assert(info1);
  assert(info2);

  /* Initialize comparison value */
  *cmp_value = 0;

  /* Compare under VOL connector classes */
  H5VLcmp_connector_cls(cmp_value, info1->under_vol_id, info2->under_vol_id);
  if (*cmp_value != 0)
    return 0;

  /* Compare under VOL connector info objects */
  H5VLcmp_connector_info(cmp_value, info1->under_vol_id, info1->under_vol_info, info2->under_vol_info);
  if (*cmp_value != 0)
    return 0;

  return 0;
} /* end openvisus_vol_info_cmp() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_info_free(void* _info)
{
  openvisus_vol_info_t* info = (openvisus_vol_info_t*)_info;
  hid_t err_id;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_info_free");
#endif

  err_id = H5Eget_current_stack();

  /* Release underlying VOL ID and info */
  if (info->under_vol_info)
    H5VLfree_connector_info(info->under_vol_id, info->under_vol_info);
  H5Idec_ref(info->under_vol_id);

  H5Eset_current_stack(err_id);

  /* Free info object itself */
  free(info);

  return 0;
} /* end openvisus_vol_info_free() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_info_to_str(const void* _info, char** str)
{
  const openvisus_vol_info_t* info = (const openvisus_vol_info_t*)_info;
  H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
  char* under_vol_string = NULL;
  size_t under_vol_str_len = 0;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_info_to_str");
#endif

  /* Get value and string for underlying VOL connector */
  H5VLget_value(info->under_vol_id, &under_value);
  H5VLconnector_info_to_str(info->under_vol_info, info->under_vol_id, &under_vol_string);

  /* Determine length of underlying VOL info string */
  if (under_vol_string)
    under_vol_str_len = strlen(under_vol_string);

  /* Allocate space for our info */
  *str = (char*)H5allocate_memory(32 + under_vol_str_len, (hbool_t)0);
  assert(*str);

  /* Encode our info
   * Normally we'd use snprintf() here for a little extra safety, but that
   * call had problems on Windows until recently. So, to be as platform-independent
   * as we can, we're using sprintf() instead.
   */
  sprintf(*str, "under_vol=%u;under_info={%s}", (unsigned)under_value, (under_vol_string ? under_vol_string : ""));

  return 0;
} /* end openvisus_vol_info_to_str() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_str_to_info(const char* str, void** _info)
{
  openvisus_vol_info_t* info;
  unsigned under_vol_value;
  const char* under_vol_info_start, * under_vol_info_end;
  hid_t under_vol_id;
  void* under_vol_info = NULL;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_str_to_info");
#endif

  /* Retrieve the underlying VOL connector value and info */
  sscanf(str, "under_vol=%u;", &under_vol_value);
  under_vol_id = H5VLregister_connector_by_value((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
  under_vol_info_start = strchr(str, '{');
  under_vol_info_end = strrchr(str, '}');
  assert(under_vol_info_end > under_vol_info_start);
  if (under_vol_info_end != (under_vol_info_start + 1)) {
    char* under_vol_info_str;

    under_vol_info_str = (char*)malloc((size_t)(under_vol_info_end - under_vol_info_start));
    memcpy(under_vol_info_str, under_vol_info_start + 1, (size_t)((under_vol_info_end - under_vol_info_start) - 1));
    *(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

    H5VLconnector_str_to_info(under_vol_info_str, under_vol_id, &under_vol_info);

    free(under_vol_info_str);
  } /* end else */

  /* Allocate new pass-through VOL connector info and set its fields */
  info = (openvisus_vol_info_t*)calloc(1, sizeof(openvisus_vol_info_t));
  info->under_vol_id = under_vol_id;
  info->under_vol_info = under_vol_info;

  /* Set return value */
  *_info = info;

  return 0;
} /* end openvisus_vol_str_to_info() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static void*
openvisus_vol_get_object(const void* obj)
{
  const openvisus_vol_t* o = (const openvisus_vol_t*)obj;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_get_object");
#endif

  return H5VLget_object(o->under, o->under_vol_id);
} 


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_get_wrap_ctx(const void* obj, void** wrap_ctx)
{
  const openvisus_vol_t* o = (const openvisus_vol_t*)obj;
  openvisus_vol_wrap_ctx_t* new_wrap_ctx;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_get_wrap_ctx");
#endif

  /* Allocate new VOL object wrapping context for the  connector */
  new_wrap_ctx = (openvisus_vol_wrap_ctx_t*)calloc(1, sizeof(openvisus_vol_wrap_ctx_t));

  /* Increment reference count on underlying VOL ID, and copy the VOL info */
  new_wrap_ctx->under_vol_id = o->under_vol_id;
  H5Iinc_ref(new_wrap_ctx->under_vol_id);
  H5VLget_wrap_ctx(o->under, o->under_vol_id, &new_wrap_ctx->under_wrap_ctx);

  /* Set wrap context to return */
  *wrap_ctx = new_wrap_ctx;

  return 0;
} /* end openvisus_vol_get_wrap_ctx() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:      Success:    Pointer to wrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void*
openvisus_vol_wrap_object(void* obj, H5I_type_t obj_type, void* _wrap_ctx)
{
  openvisus_vol_wrap_ctx_t* wrap_ctx = (openvisus_vol_wrap_ctx_t*)_wrap_ctx;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_wrap_object");
#endif

  /* Wrap the object with the underlying VOL */
  void*  under = H5VLwrap_object(obj, obj_type, wrap_ctx->under_vol_id, wrap_ctx->under_wrap_ctx);
  if (!under)
    return nullptr;

  openvisus_vol_t* ret = nullptr;

#if USE_OPENVISUS_DATASET
  if (obj_type == H5I_DATASET)
  {
    String filename = ""; //TODO how to set the filename????
    VisusReleaseAssert(false);
    SharedPtr<Dataset> db = LoadDataset(filename); VisusReleaseAssert(db);
    ret = openvisus_vol_new_obj(under, wrap_ctx->under_vol_id, db);
  }
#endif

  if (!ret)
    ret = openvisus_vol_new_obj(under, wrap_ctx->under_vol_id);

  return ret;
} 


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_unwrap_object
 *
 * Purpose:     Unwrap a wrapped object, discarding the wrapper, but returning
 *		underlying object.
 *
 * Return:      Success:    Pointer to unwrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void*
openvisus_vol_unwrap_object(void* obj)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_unwrap_object");
#endif

  /* Unrap the object with the underlying VOL */
  under = H5VLunwrap_object(o->under, o->under_vol_id);

  if (under)
    openvisus_vol_free_obj(o);

  return under;
} /* end openvisus_vol_unwrap_object() */


/*---------------------------------------------------------------------------
 * Function:    openvisus_vol_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_free_wrap_ctx(void* _wrap_ctx)
{
  openvisus_vol_wrap_ctx_t* wrap_ctx = (openvisus_vol_wrap_ctx_t*)_wrap_ctx;
  hid_t err_id;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_free_wrap_ctx");
#endif

  err_id = H5Eget_current_stack();

  /* Release underlying VOL ID and wrap context */
  if (wrap_ctx->under_wrap_ctx)
    H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
  H5Idec_ref(wrap_ctx->under_vol_id);

  H5Eset_current_stack(err_id);

  /* Free wrap context object itself */
  free(wrap_ctx);

  return 0;
} /* end openvisus_vol_free_wrap_ctx() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_create
 *
 * Purpose:     Creates an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void*
openvisus_vol_attr_create(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name, hid_t type_id, hid_t space_id, hid_t acpl_id,
  hid_t aapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* attr;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_create");
#endif

  under = H5VLattr_create(o->under, loc_params, o->under_vol_id, name, type_id, space_id, acpl_id, aapl_id, dxpl_id, req);
  if (under) {
    attr = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    attr = NULL;

  return (void*)attr;
} /* end openvisus_vol_attr_create() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_open
 *
 * Purpose:     Opens an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void*
openvisus_vol_attr_open(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name, hid_t aapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* attr;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_open");
#endif

  under = H5VLattr_open(o->under, loc_params, o->under_vol_id, name, aapl_id, dxpl_id, req);
  if (under) {
    attr = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    attr = NULL;

  return (void*)attr;
} /* end openvisus_vol_attr_open() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_read
 *
 * Purpose:     Reads data from attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_read(void* attr, hid_t mem_type_id, void* buf,
  hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)attr;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_read");
#endif

  ret_value = H5VLattr_read(o->under, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_attr_read() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_write
 *
 * Purpose:     Writes data to attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_write(void* attr, hid_t mem_type_id, const void* buf,
  hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)attr;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_write");
#endif

  ret_value = H5VLattr_write(o->under, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_attr_write() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_get
 *
 * Purpose:     Gets information about an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_get(void* obj, H5VL_attr_get_t get_type, hid_t dxpl_id,
  void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_get");
#endif

  ret_value = H5VLattr_get(o->under, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_attr_get() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_specific
 *
 * Purpose:     Specific operation on attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_specific(void* obj, const H5VL_loc_params_t* loc_params,
  H5VL_attr_specific_t specific_type, hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_specific");
#endif

  ret_value = H5VLattr_specific(o->under, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_attr_specific() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_optional
 *
 * Purpose:     Perform a connector-specific operation on an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_optional(void* obj, H5VL_attr_optional_t opt_type,
  hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_optional");
#endif

  ret_value = H5VLattr_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_attr_optional() */


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_attr_close
 *
 * Purpose:     Closes an attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1, attr not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_attr_close(void* attr, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)attr;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_attr_close");
#endif

  ret_value = H5VLattr_close(o->under, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying attribute was closed */
  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;
} /* end openvisus_vol_attr_close() */


#if USE_OPENVISUS_DATASET


//////////////////////////////////////////////////////////////////////////////
class OpenVisusHDF5Utils
{
public:

  //ToDType
  static DType ToDType(hid_t type_id)
  {
    size_t      type_size = H5Tget_size(type_id);
    H5T_class_t type_class = H5Tget_class(type_id);
    H5T_sign_t   type_sign = H5Tget_sign(type_id);

    String ret = "";

    if (type_class == H5T_INTEGER)
    {
      switch (type_size)
      {
      case 1: ret = "int8"; break;
      case 2: ret = "int16"; break;
      case 4: ret = "int32"; break;
      case 8: ret = "int64"; break;
      default: break;
      }

      if (!ret.empty() && !type_sign)
        ret = "u" + ret;
    }
    else if (type_class == H5T_FLOAT)
    {
      switch (type_size)
      {
      case 4: ret = "float32"; break;
      case 8: ret = "float64"; break;
      default: break;
      }
    }

    VisusReleaseAssert(!ret.empty());
    return DType::fromString(ret);
  }


  //ToTypeId
  static hid_t ToTypeId(DType dtype)
  {
    VisusReleaseAssert(dtype.ncomponents() == 1);

    if (dtype == DTypes::INT8)  return H5T_NATIVE_CHAR;
    if (dtype == DTypes::INT16) return H5T_NATIVE_SHORT;
    if (dtype == DTypes::INT32) return H5T_NATIVE_INT;
    if (dtype == DTypes::INT64) return H5T_NATIVE_LLONG;

    if (dtype == DTypes::UINT8)  return H5T_NATIVE_UCHAR;
    if (dtype == DTypes::UINT16) return H5T_NATIVE_USHORT;
    if (dtype == DTypes::UINT32) return H5T_NATIVE_UINT;
    if (dtype == DTypes::UINT64) return H5T_NATIVE_ULLONG;

    if (dtype == DTypes::FLOAT32)  return H5T_NATIVE_FLOAT;
    if (dtype == DTypes::FLOAT64)  return H5T_NATIVE_DOUBLE;

    VisusReleaseAssert(false);
    return 0;
  }

  //executeQuery
  static herr_t executeQuery(SharedPtr<Dataset> db, int mode, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void* buf, void** req)
  {
    BoxNi world_box = db->getLogicBox();
    int pdim = db->getPointDim();
    auto access = db->createAccess();
    auto dtype = db->getField().dtype;

    //TODO casting for memory
    VisusReleaseAssert(dtype == ToDType(mem_type_id));

    auto mem_space_type = mem_space_id == H5S_ALL ? H5S_SEL_ALL : H5Sget_select_type(mem_space_id);
    auto file_space_type = file_space_id == H5S_ALL ? H5S_SEL_ALL : H5Sget_select_type(file_space_id);

    //TODO deal with subarea (i.e. subarray) inside destination memory buffer (i.e. only onememcpy wound not be enough)
    VisusReleaseAssert(mem_space_type == H5S_SEL_ALL);

    BoxNi logic_box(pdim);

    switch (file_space_type)
    {
    case H5S_SEL_NONE:
      return 0;

    case H5S_SEL_POINTS:
    {
      VisusReleaseAssert(false);
      return -1; //not supported
    }
    case H5S_SEL_ALL:
    {
      logic_box = world_box;
      break;
    }

    case H5S_SEL_HYPERSLABS:
    {
      //TODO: other cases
#if 0
      hssize_t nblock = H5Sget_select_hyper_nblocks(file_space_id);
      VisusReleaseAssert(nblock > 0);
      VisusReleaseAssert(H5Sget_select_hyper_nblocks(mem_space_id) == nblock);

      //coordinates:= start_block1 start_block2 start_block3... corner_block1 corner_block2 corner_block3
      //   where each item has pdim values
      std::vector<hsize_t> coordinates(pdim * 2 * nblock);
      VisusReleaseAssert(H5Sget_select_hyper_blocklist(file_space_id, 0, nblock, &coordinates[0]) == 0);

      std::vector<hsize_t> mem_coordinates(pdim * 2 * nblock);
      VisusReleaseAssert(H5Sget_select_hyper_blocklist(mem_space_id, 0, nblock, &mem_coordinates[0]) == 0);

      //see http://davis.lbl.gov/Manuals/HDF5-1.6.1/RM_H5S.html#Dataspace-SelectHyperBlockList
      for (int Block = 0; Block < nblock; Block++)
      {
        auto Offset = coordinates.begin() + pdim * 2 * Block;

        auto start = std::vector<Int64>(Offset + 0 * pdim, Offset + 1 * pdim);
        auto corner = std::vector<Int64>(Offset + 1 * pdim, Offset + 2 * pdim);

        std::reverse(start.begin(), start.end());
        std::reverse(corner.begin(), corner.end());

        //corner is included
        for (auto& val : corner)
          val++;
#endif

        //todo other cases
        VisusReleaseAssert(H5Sis_regular_hyperslab(file_space_id));

        //note count is the number of blocks, block is the block dims
        std::vector< hsize_t> start(pdim), stride(pdim), count(pdim), block(pdim);
        VisusReleaseAssert(H5Sget_regular_hyperslab(file_space_id, &start[0], &stride[0], &count[0], &block[0]) == 0);

        //PrintInfo("start", StringUtils::join(start));
        //PrintInfo("stride", StringUtils::join(stride));
        //PrintInfo("count", StringUtils::join(count));
        //PrintInfo("block", StringUtils::join(block));

        std::reverse(start.begin(), start.end());
        std::reverse(stride.begin(), stride.end());
        std::reverse(count.begin(), count.end());
        std::reverse(block.begin(), block.end());

        for (int D = 0; D < pdim; D++)
        {
          VisusReleaseAssert(count[D] >= 1);
          VisusReleaseAssert(count[D] == 1 || (count[D] > 1 && stride[D] == block[D])); //need one continuos query
          logic_box.p1[D] = start[D];
          logic_box.p2[D] = start[D] + count[D] * block[D];
        }

        break;
    }

    default:
    {
      VisusReleaseAssert(false);
      return -1;
    }
    }

    auto dims = logic_box.size();
    PrintInfo("Executing query", logic_box, "dims", dims, "mode", String(1, mode));

    //I can do this just because mem_space_type==H5S_SEL_ALL TODO Extend!
    auto heap = HeapMemory::createUnmanaged((Uint8*)buf, dtype.getByteSize(dims));

    auto query = db->createBoxQuery(logic_box, mode);
    db->beginBoxQuery(query);
    VisusReleaseAssert(query->isRunning());
    query->buffer = Array(dims, dtype, heap);
    VisusReleaseAssert(db->executeBoxQuery(access, query));

    return 0;
  }
};

#endif


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void*
openvisus_vol_dataset_create(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name_, 
  hid_t lcpl_id, 
  hid_t type_id, 
  hid_t space_id,
  hid_t dcpl_id, 
  hid_t dapl_id, 
  hid_t dxpl_id, 
  void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  String name = name_;

//#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_create");
//#endif

  void* under = H5VLdataset_create(o->under, loc_params, o->under_vol_id, name.c_str(), lcpl_id, type_id, space_id, dcpl_id, dapl_id, dxpl_id, req);

  if (!under)
    return nullptr;

#if USE_OPENVISUS_DATASET

  //shape
  std::vector<hsize_t> shape(H5S_MAX_RANK);
  int _ndim = H5Sget_simple_extent_dims(space_id, &shape[0], NULL);
  shape.resize(_ndim);

  //dtype
  DType dtype = OpenVisusHDF5Utils::ToDType(type_id);;
  PointNi dims;

  //shape==(height,width) 2D dataset
  if (shape.size() == 2)
  {
    auto width = shape[1];
    auto height = shape[0];
    dims = PointNi(width, height);
  }
  //shape==(depth,height,width) 3D dataset (note: 2D with multiple channels not supported right now)
  else if (shape.size() == 3)
  {
    auto depth = shape[0];
    auto height = shape[1];
    auto width = shape[2];
    dims = PointNi(width, height, depth);
  }
  //shape==(depth,height,width,nchannels)
  else if (shape.size() == 4)
  {
    auto depth = shape[0];
    auto height = shape[1];
    auto width = shape[2];
    auto nchannels = shape[3];
    dtype = DType((int)nchannels, dtype);
    dims = PointNi(width, height, depth);
  }
  else
  {
    VisusAssert(false);
  }

  // Filters not supported
  if (int nfilter = H5Pget_nfilters(dcpl_id))
    PrintWarning("filters not supported");

  //TODO
  /*
  - Continuos vs Chunked does it matter to set the dataset propery? Or should I set virtual
    - see https://github.com/DataLib-ECP/vol-log-based/blob/b13778efd9e0c79135a9d7352104985408078d45/src/H5VL_log_dataset.cpp
    - Example: H5Pset_layout (dcpl_id, H5D_CONTIGUOUS);
  - filename should not be only the name
  - automatically set chunked
  - add other coarse-dataset
  */

  String filename = StringUtils::endsWith(name, ".idx")? name : name + ".idx";
  PrintInfo(filename, "!!!");

  //create the idx file if it does not exists
  if (!FileUtils::existsFile(filename))
  {
    IdxFile idxfile;
    idxfile.logic_box = BoxNi(PointNi::zero(dims.getPointDim()), dims);
    idxfile.fields.push_back(Field("myfield", dtype));
    idxfile.save(filename);
  }
  else
  {
    PrintWarning("file", filename, "already exists. It MUST be dtype/dimension compatible");
  }

  //equivalent to open
  auto db = LoadIdxDataset(filename);
  VisusReleaseAssert(db);

  PrintInfo("Created dataset", "filename", filename, "name", name, "dtype", dtype, "dims", dims, "under", (Uint64)under);

  auto ret=openvisus_vol_new_obj(under, o->under_vol_id, db);

#else
  auto ret = openvisus_vol_new_obj(under, o->under_vol_id);
#endif

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret;
} 


/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void*
openvisus_vol_dataset_open(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name_, hid_t dapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  String name = name_;
  
//#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_open");
//#endif

  void* under = H5VLdataset_open(o->under, loc_params, o->under_vol_id, name.c_str(), dapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

#if USE_OPENVISUS_DATASET

  //open visus db
  String filename = StringUtils::endsWith(name, ".idx") ? name : name + ".idx";
  SharedPtr<Dataset> db = LoadDataset(filename); VisusReleaseAssert(db);
  auto ret=openvisus_vol_new_obj(under, o->under_vol_id, db);

#else
  auto ret = openvisus_vol_new_obj(under, o->under_vol_id);
#endif

  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret;

} /* end openvisus_vol_dataset_open() */



/*-------------------------------------------------------------------------
 * Function:    openvisus_vol_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
openvisus_vol_dataset_read(void* o_, hid_t mem_type_id, hid_t mem_space_id,
  hid_t file_space_id, hid_t plist_id, void* buf, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)o_;
  
  //#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_read","o",(Int64)o);
  //#endif

#if USE_OPENVISUS_DATASET
  herr_t ret_value = OpenVisusHDF5Utils::executeQuery(*o->db, 'r', mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
#else
   herr_t ret_value = H5VLdataset_read(o->under, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
#endif

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;

} /* end openvisus_vol_dataset_read() */



/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_dataset_write
  *
  * Purpose:     Writes data elements from a buffer into a dataset.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t openvisus_vol_dataset_write(void* o_, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void* buf, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)o_;
    
  //#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_write");
  //#endif

#if USE_OPENVISUS_DATASET
  herr_t ret_value = OpenVisusHDF5Utils::executeQuery(*o->db, 'w', mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
#else
   herr_t ret_value = H5VLdataset_write(o->under, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
#endif

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;

} /* end openvisus_vol_dataset_write() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_dataset_get
  *
  * Purpose:     Gets information about a dataset
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t openvisus_vol_dataset_get(void* dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)dset;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_get", (int)get_type);
#endif

  //use the original dataset
  herr_t ret_value;
  ret_value = H5VLdataset_get(o->under, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;

} /* end openvisus_vol_dataset_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_dataset_specific
  *
  * Purpose:     Specific operation on a dataset
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_dataset_specific(void* obj, H5VL_dataset_specific_t specific_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;

 #if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  hid_t under_vol_id = o->under_vol_id;

  herr_t ret_value = H5VLdataset_specific(o->under, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;

} /* end openvisus_vol_dataset_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_dataset_optional
  *
  * Purpose:     Perform a connector-specific operation on a dataset
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_dataset_optional(void* obj, H5VL_dataset_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_optional");
#endif

   herr_t ret_value = H5VLdataset_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;

} /* end openvisus_vol_dataset_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_dataset_close
  *
  * Purpose:     Closes a dataset.
  *
  * Return:      Success:    0
  *              Failure:    -1, dataset not closed.
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_dataset_close(void* dset, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)dset;

  //#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_dataset_close");
  //#endif


   herr_t ret_value = H5VLdataset_close(o->under, o->under_vol_id, dxpl_id, req);

#if USE_OPENVISUS_DATASET
   if (ret_value >= 0)
   {
     SharedPtr<Dataset> db = *o->db;
     PrintInfo("Closing database", "dims", db->getLogicBox().size(), "dtype", db->getField().dtype);
   }
#endif

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying dataset was closed */
  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;

} /* end openvisus_vol_dataset_close() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_commit
  *
  * Purpose:     Commits a datatype inside a container.
  *
  * Return:      Success:    Pointer to datatype object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_datatype_commit(void* obj, const H5VL_loc_params_t * loc_params,
    const char* name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
    hid_t dxpl_id, void** req)
{
  openvisus_vol_t* dt;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_commit");
#endif

  under = H5VLdatatype_commit(o->under, loc_params, o->under_vol_id, name, type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
  if (under) {
    dt = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dt = NULL;

  return (void*)dt;
} /* end openvisus_vol_datatype_commit() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_open
  *
  * Purpose:     Opens a named datatype inside a container.
  *
  * Return:      Success:    Pointer to datatype object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_datatype_open(void* obj, const H5VL_loc_params_t * loc_params,
    const char* name, hid_t tapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* dt;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_open");
#endif

  under = H5VLdatatype_open(o->under, loc_params, o->under_vol_id, name, tapl_id, dxpl_id, req);
  if (under) {
    dt = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dt = NULL;

  return (void*)dt;
} /* end openvisus_vol_datatype_open() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_get
  *
  * Purpose:     Get information about a datatype
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_datatype_get(void* dt, H5VL_datatype_get_t get_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)dt;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_get");
#endif

  ret_value = H5VLdatatype_get(o->under, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_datatype_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_specific
  *
  * Purpose:     Specific operations for datatypes
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_datatype_specific(void* obj, H5VL_datatype_specific_t specific_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLdatatype_specific(o->under, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_datatype_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_optional
  *
  * Purpose:     Perform a connector-specific operation on a datatype
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_datatype_optional(void* obj, H5VL_datatype_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_optional");
#endif

  ret_value = H5VLdatatype_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_datatype_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_datatype_close
  *
  * Purpose:     Closes a datatype.
  *
  * Return:      Success:    0
  *              Failure:    -1, datatype not closed.
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_datatype_close(void* dt, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)dt;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_datatype_close");
#endif

  assert(o->under);

  ret_value = H5VLdatatype_close(o->under, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying datatype was closed */
  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;
} /* end openvisus_vol_datatype_close() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_create
  *
  * Purpose:     Creates a container using this connector
  *
  * Return:      Success:    Pointer to a file object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_file_create(const char* name, unsigned flags, hid_t fcpl_id,
    hid_t fapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_info_t* info;
  openvisus_vol_t* file;
  hid_t under_fapl_id;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_create");
#endif

  /* Get copy of our VOL info from FAPL */
  H5Pget_vol_info(fapl_id, (void**)&info);

  /* Make sure we have info about the underlying VOL to be used */
  if (!info)
    return NULL;

  /* Copy the FAPL */
  under_fapl_id = H5Pcopy(fapl_id);

  /* Set the VOL ID and info for the underlying FAPL */
  H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

  /* Open the file with the underlying VOL connector */
  under = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, req);
  if (under) {
    file = openvisus_vol_new_obj(under, info->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, info->under_vol_id);
  } /* end if */
  else
    file = NULL;

  /* Close underlying FAPL */
  H5Pclose(under_fapl_id);

  /* Release copy of our VOL info */
  openvisus_vol_info_free(info);

  return (void*)file;
} /* end openvisus_vol_file_create() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_open
  *
  * Purpose:     Opens a container created with this connector
  *
  * Return:      Success:    Pointer to a file object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_file_open(const char* name, unsigned flags, hid_t fapl_id,
    hid_t dxpl_id, void** req)
{
  openvisus_vol_info_t* info;
  openvisus_vol_t* file;
  hid_t under_fapl_id;
  void* under;
#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_open");
#endif

  /* Get copy of our VOL info from FAPL */
  H5Pget_vol_info(fapl_id, (void**)&info);

  /* Make sure we have info about the underlying VOL to be used */
  if (!info)
    return NULL;

  /* Copy the FAPL */
  under_fapl_id = H5Pcopy(fapl_id);

  /* Set the VOL ID and info for the underlying FAPL */
  H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

  /* Open the file with the underlying VOL connector */
  under = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, req);
  if (under) {
    file = openvisus_vol_new_obj(under, info->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, info->under_vol_id);
  } /* end if */
  else
    file = NULL;

  /* Close underlying FAPL */
  H5Pclose(under_fapl_id);

  /* Release copy of our VOL info */
  openvisus_vol_info_free(info);

  return (void*)file;
} /* end openvisus_vol_file_open() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_get
  *
  * Purpose:     Get info about a file
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_file_get(void* file, H5VL_file_get_t get_type, hid_t dxpl_id,
    void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)file;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_get");
#endif

  ret_value = H5VLfile_get(o->under, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_file_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_specific_reissue
  *
  * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
  *              file specific callback to the underlying VOL connector.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_file_specific_reissue(void* obj, hid_t connector_id,
    H5VL_file_specific_t specific_type, hid_t dxpl_id, void** req, ...)
{
  va_list arguments;
  herr_t ret_value;

  va_start(arguments, req);
  ret_value = H5VLfile_specific(obj, connector_id, specific_type, dxpl_id, req, arguments);
  va_end(arguments);

  return ret_value;
} /* end openvisus_vol_file_specific_reissue() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_specific
  *
  * Purpose:     Specific operation on file
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_file_specific(void* file, H5VL_file_specific_t specific_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)file;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_specific");
#endif

  /* Unpack arguments to get at the child file pointer when mounting a file */
  if (specific_type == H5VL_FILE_MOUNT) {
    H5I_type_t loc_type;
    const char* name;
    openvisus_vol_t* child_file;
    hid_t plist_id;

    /* Retrieve parameters for 'mount' operation, so we can unwrap the child file */
    loc_type = (H5I_type_t)va_arg(arguments, int); /* enum work-around */
    name = va_arg(arguments, const char*);
    child_file = (openvisus_vol_t*)va_arg(arguments, void*);
    plist_id = va_arg(arguments, hid_t);

    /* Keep the correct underlying VOL ID for possible async request token */
    under_vol_id = o->under_vol_id;

    /* Re-issue 'file specific' call, using the unwrapped pieces */
    ret_value = openvisus_vol_file_specific_reissue(o->under, o->under_vol_id, specific_type, dxpl_id, req, (int)loc_type, name, child_file->under, plist_id);
  } /* end if */
  else if (specific_type == H5VL_FILE_IS_ACCESSIBLE || specific_type == H5VL_FILE_DELETE) {
    openvisus_vol_info_t* info;
    hid_t fapl_id, under_fapl_id;
    const char* name;
    htri_t* ret;

    /* Get the arguments for the 'is accessible' check */
    fapl_id = va_arg(arguments, hid_t);
    name = va_arg(arguments, const char*);
    ret = va_arg(arguments, htri_t*);

    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void**)&info);

    /* Make sure we have info about the underlying VOL to be used */
    if (!info)
      return (-1);

    /* Copy the FAPL */
    under_fapl_id = H5Pcopy(fapl_id);

    /* Set the VOL ID and info for the underlying FAPL */
    H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

    /* Keep the correct underlying VOL ID for possible async request token */
    under_vol_id = info->under_vol_id;

    /* Re-issue 'file specific' call */
    ret_value = openvisus_vol_file_specific_reissue(NULL, info->under_vol_id, specific_type, dxpl_id, req, under_fapl_id, name, ret);

    /* Close underlying FAPL */
    H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    openvisus_vol_info_free(info);
  } /* end else-if */
  else {
    va_list my_arguments;

    /* Make a copy of the argument list for later, if reopening */
    if (specific_type == H5VL_FILE_REOPEN)
      va_copy(my_arguments, arguments);

    /* Keep the correct underlying VOL ID for possible async request token */
    under_vol_id = o->under_vol_id;

    ret_value = H5VLfile_specific(o->under, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Wrap file struct pointer, if we reopened one */
    if (specific_type == H5VL_FILE_REOPEN) {
      if (ret_value >= 0) {
        void** ret = va_arg(my_arguments, void**);

        if (ret && *ret)
          *ret = openvisus_vol_new_obj(*ret, o->under_vol_id);
      } /* end if */

      /* Finish use of copied vararg list */
      va_end(my_arguments);
    } /* end if */
  } /* end else */

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_file_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_optional
  *
  * Purpose:     Perform a connector-specific operation on a file
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_file_optional(void* file, H5VL_file_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)file;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_optional");
#endif

  ret_value = H5VLfile_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_file_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_file_close
  *
  * Purpose:     Closes a file.
  *
  * Return:      Success:    0
  *              Failure:    -1, file not closed.
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_file_close(void* file, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)file;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_file_close");
#endif

  ret_value = H5VLfile_close(o->under, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying file was closed */
  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;
} /* end openvisus_vol_file_close() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_create
  *
  * Purpose:     Creates a group inside a container
  *
  * Return:      Success:    Pointer to a group object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_group_create(void* obj, const H5VL_loc_params_t * loc_params,
    const char* name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id,
    hid_t dxpl_id, void** req)
{
  openvisus_vol_t* group;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_create");
#endif

  under = H5VLgroup_create(o->under, loc_params, o->under_vol_id, name, lcpl_id, gcpl_id, gapl_id, dxpl_id, req);
  if (under) {
    group = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    group = NULL;

  return (void*)group;
} /* end openvisus_vol_group_create() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_open
  *
  * Purpose:     Opens a group inside a container
  *
  * Return:      Success:    Pointer to a group object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*
  openvisus_vol_group_open(void* obj, const H5VL_loc_params_t * loc_params,
    const char* name, hid_t gapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* group;
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  void* under;
  printf("%s:%d: openvisus_vol_group_open is called.\n", __func__, __LINE__);
#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_open");
#endif

  under = H5VLgroup_open(o->under, loc_params, o->under_vol_id, name, gapl_id, dxpl_id, req);
  if (under) {
    group = openvisus_vol_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req)
      *req = openvisus_vol_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    group = NULL;

  return (void*)group;
} /* end openvisus_vol_group_open() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_get
  *
  * Purpose:     Get info about a group
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_group_get(void* obj, H5VL_group_get_t get_type, hid_t dxpl_id,
    void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_get");
#endif

  ret_value = H5VLgroup_get(o->under, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_group_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_specific
  *
  * Purpose:     Specific operation on a group
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_group_specific(void* obj, H5VL_group_specific_t specific_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLgroup_specific(o->under, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_group_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_optional
  *
  * Purpose:     Perform a connector-specific operation on a group
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_group_optional(void* obj, H5VL_group_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_optional");
#endif

  ret_value = H5VLgroup_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_group_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_group_close
  *
  * Purpose:     Closes a group.
  *
  * Return:      Success:    0
  *              Failure:    -1, group not closed.
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_group_close(void* grp, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)grp;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_group_close");
#endif

  ret_value = H5VLgroup_close(o->under, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying file was closed */
  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;
} /* end openvisus_vol_group_close() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_create_reissue
  *
  * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
  *              link create callback to the underlying VOL connector.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_create_reissue(H5VL_link_create_type_t create_type,
    void* obj, const H5VL_loc_params_t * loc_params, hid_t connector_id,
    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void** req, ...)
{
  va_list arguments;
  herr_t ret_value;

  va_start(arguments, req);
  ret_value = H5VLlink_create(create_type, obj, loc_params, connector_id, lcpl_id, lapl_id, dxpl_id, req, arguments);
  va_end(arguments);

  return ret_value;
} /* end openvisus_vol_link_create_reissue() */

/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_create
  *
  * Purpose:     Creates a hard / soft / UD / external link.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_create(H5VL_link_create_type_t create_type, void* obj,
    const H5VL_loc_params_t * loc_params, hid_t lcpl_id, hid_t lapl_id,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_create");
#endif

  /* Try to retrieve the "under" VOL id */
  if (o)
    under_vol_id = o->under_vol_id;

  /* Fix up the link target object for hard link creation */
  if (H5VL_LINK_CREATE_HARD == create_type) {
    void* cur_obj;
    H5VL_loc_params_t* cur_params;

    /* Retrieve the object & loc params for the link target */
    cur_obj = va_arg(arguments, void*);
    cur_params = va_arg(arguments, H5VL_loc_params_t*);

    /* If it's a non-NULL pointer, find the 'under object' and re-set the property */
    if (cur_obj) {
      /* Check if we still need the "under" VOL ID */
      if (under_vol_id < 0)
        under_vol_id = ((openvisus_vol_t*)cur_obj)->under_vol_id;

      /* Set the object for the link target */
      cur_obj = ((openvisus_vol_t*)cur_obj)->under;
    } /* end if */

    /* Re-issue 'link create' call, using the unwrapped pieces */
    ret_value = openvisus_vol_link_create_reissue(create_type, (o ? o->under : NULL), loc_params, under_vol_id, lcpl_id, lapl_id, dxpl_id, req, cur_obj, cur_params);
  } /* end if */
  else
    ret_value = H5VLlink_create(create_type, (o ? o->under : NULL), loc_params, under_vol_id, lcpl_id, lapl_id, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_create() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_copy
  *
  * Purpose:     Renames an object within an HDF5 container and copies it to a new
  *              group.  The original name SRC is unlinked from the group graph
  *              and then inserted with the new name DST (which can specify a
  *              new path for the object) as an atomic operation. The names
  *              are interpreted relative to SRC_LOC_ID and
  *              DST_LOC_ID, which are either file IDs or group ID.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_copy(void* src_obj, const H5VL_loc_params_t * loc_params1,
    void* dst_obj, const H5VL_loc_params_t * loc_params2, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o_src = (openvisus_vol_t*)src_obj;
  openvisus_vol_t* o_dst = (openvisus_vol_t*)dst_obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_copy");
#endif

  /* Retrieve the "under" VOL id */
  if (o_src)
    under_vol_id = o_src->under_vol_id;
  else if (o_dst)
    under_vol_id = o_dst->under_vol_id;
  assert(under_vol_id > 0);

  ret_value = H5VLlink_copy((o_src ? o_src->under : NULL), loc_params1, (o_dst ? o_dst->under : NULL), loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_copy() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_move
  *
  * Purpose:     Moves a link within an HDF5 file to a new group.  The original
  *              name SRC is unlinked from the group graph
  *              and then inserted with the new name DST (which can specify a
  *              new path for the object) as an atomic operation. The names
  *              are interpreted relative to SRC_LOC_ID and
  *              DST_LOC_ID, which are either file IDs or group ID.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_move(void* src_obj, const H5VL_loc_params_t * loc_params1,
    void* dst_obj, const H5VL_loc_params_t * loc_params2, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o_src = (openvisus_vol_t*)src_obj;
  openvisus_vol_t* o_dst = (openvisus_vol_t*)dst_obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_move");
#endif

  /* Retrieve the "under" VOL id */
  if (o_src)
    under_vol_id = o_src->under_vol_id;
  else if (o_dst)
    under_vol_id = o_dst->under_vol_id;
  assert(under_vol_id > 0);

  ret_value = H5VLlink_move((o_src ? o_src->under : NULL), loc_params1, (o_dst ? o_dst->under : NULL), loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_move() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_get
  *
  * Purpose:     Get info about a link
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_get(void* obj, const H5VL_loc_params_t * loc_params,
    H5VL_link_get_t get_type, hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_get");
#endif

  ret_value = H5VLlink_get(o->under, loc_params, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_specific
  *
  * Purpose:     Specific operation on a link
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_specific(void* obj, const H5VL_loc_params_t * loc_params,
    H5VL_link_specific_t specific_type, hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_specific");
#endif

  ret_value = H5VLlink_specific(o->under, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_link_optional
  *
  * Purpose:     Perform a connector-specific operation on a link
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_link_optional(void* obj, H5VL_link_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_link_optional");
#endif

  ret_value = H5VLlink_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_link_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_object_open
  *
  * Purpose:     Opens an object inside a container.
  *
  * Return:      Success:    Pointer to object
  *              Failure:    NULL
  *
  *-------------------------------------------------------------------------
  */
static void*  openvisus_vol_object_open(void* o_, const H5VL_loc_params_t * loc_params, H5I_type_t * opened_type, hid_t dxpl_id, void** req)
{
  openvisus_vol_t* o = (openvisus_vol_t*)o_;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_object_open");
#endif

  void* under = H5VLobject_open(o->under, loc_params, o->under_vol_id, opened_type, dxpl_id, req);
  if (!under)
    return nullptr;

  openvisus_vol_t* ret = nullptr;

#if USE_OPENVISUS_DATASET
  if (*opened_type == H5I_DATASET)
  {
    VisusReleaseAssert(loc_params->type== H5VL_OBJECT_BY_NAME); //TODO other cases
    String name = loc_params->loc_data.loc_by_name.name;
    String filename = StringUtils::endsWith(name, ".idx")? name : name+".idx";
    SharedPtr<Dataset> db = LoadDataset(filename); VisusReleaseAssert(db);
    ret= openvisus_vol_new_obj(under, o->under_vol_id, db);
  } 
#endif

  if  (!ret)
    ret = openvisus_vol_new_obj(under, o->under_vol_id);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return (void*)ret;
} 


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_object_copy
  *
  * Purpose:     Copies an object inside a container.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_object_copy(void* src_obj, const H5VL_loc_params_t * src_loc_params,
    const char* src_name, void* dst_obj, const H5VL_loc_params_t * dst_loc_params,
    const char* dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id,
    void** req)
{
  openvisus_vol_t* o_src = (openvisus_vol_t*)src_obj;
  openvisus_vol_t* o_dst = (openvisus_vol_t*)dst_obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_object_copy");
#endif

  ret_value = H5VLobject_copy(o_src->under, src_loc_params, src_name, o_dst->under, dst_loc_params, dst_name, o_src->under_vol_id, ocpypl_id, lcpl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o_src->under_vol_id);

  return ret_value;
} /* end openvisus_vol_object_copy() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_object_get
  *
  * Purpose:     Get info about an object
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_object_get(void* obj, const H5VL_loc_params_t * loc_params, H5VL_object_get_t get_type, hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_object_get");
#endif

  ret_value = H5VLobject_get(o->under, loc_params, o->under_vol_id, get_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_object_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_object_specific
  *
  * Purpose:     Specific operation on an object
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_object_specific(void* obj, const H5VL_loc_params_t * loc_params,
    H5VL_object_specific_t specific_type, hid_t dxpl_id, void** req,
    va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_object_specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLobject_specific(o->under, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, under_vol_id);

  return ret_value;
} /* end openvisus_vol_object_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_object_optional
  *
  * Purpose:     Perform a connector-specific operation for an object
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_object_optional(void* obj, H5VL_object_optional_t opt_type,
    hid_t dxpl_id, void** req, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_object_optional");
#endif

  ret_value = H5VLobject_optional(o->under, o->under_vol_id, opt_type, dxpl_id, req, arguments);

  /* Check for async request */
  if (req && *req)
    *req = openvisus_vol_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_object_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_introspect_get_conn_clss
  *
  * Purpose:     Query the connector class.
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_introspect_get_conn_cls(void* obj, H5VL_get_conn_lvl_t lvl,
    const H5VL_class_t * *conn_cls)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_introspect_get_conn_cls");
#endif

  /* Check for querying this connector's class */
  if (H5VL_GET_CONN_LVL_CURR == lvl) {
    *conn_cls = &openvisus_vol_g;
    ret_value = 0;
  } /* end if */
  else
    ret_value = H5VLintrospect_get_conn_cls(o->under, o->under_vol_id,
      lvl, conn_cls);

  return ret_value;
} /* end openvisus_vol_introspect_get_conn_cls() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_introspect_opt_query
  *
  * Purpose:     Query if an optional operation is supported by this connector
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_introspect_opt_query(void* obj, H5VL_subclass_t cls,
    int opt_type, hbool_t * supported)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_introspect_opt_query");
#endif

  ret_value = H5VLintrospect_opt_query(o->under, o->under_vol_id, cls,
    opt_type, supported);

  return ret_value;
} /* end openvisus_vol_introspect_opt_query() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_wait
  *
  * Purpose:     Wait (with a timeout) for an async operation to complete
  *
  * Note:        Releases the request if the operation has completed and the
  *              connector callback succeeds
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_wait(void* obj, uint64_t timeout,
    H5ES_status_t * status)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_wait");
#endif

  ret_value = H5VLrequest_wait(o->under, o->under_vol_id, timeout, status);

  return ret_value;
} /* end openvisus_vol_request_wait() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_notify
  *
  * Purpose:     Registers a user callback to be invoked when an asynchronous
  *              operation completes
  *
  * Note:        Releases the request, if connector callback succeeds
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_notify(void* obj, H5VL_request_notify_t cb, void* ctx)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_notify");
#endif

  ret_value = H5VLrequest_notify(o->under, o->under_vol_id, cb, ctx);

  return ret_value;
} /* end openvisus_vol_request_notify() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_cancel
  *
  * Purpose:     Cancels an asynchronous operation
  *
  * Note:        Releases the request, if connector callback succeeds
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_cancel(void* obj)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_cancel");
#endif

  ret_value = H5VLrequest_cancel(o->under, o->under_vol_id);

  return ret_value;
} /* end openvisus_vol_request_cancel() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_specific_reissue
  *
  * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
  *              request specific callback to the underlying VOL connector.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_specific_reissue(void* obj, hid_t connector_id,
    H5VL_request_specific_t specific_type, ...)
{
  va_list arguments;
  herr_t ret_value;

  va_start(arguments, specific_type);
  ret_value = H5VLrequest_specific(obj, connector_id, specific_type, arguments);
  va_end(arguments);

  return ret_value;
} /* end openvisus_vol_request_specific_reissue() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_specific
  *
  * Purpose:     Specific operation on a request
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_specific(void* obj, H5VL_request_specific_t specific_type,
    va_list arguments)
{
  herr_t ret_value = -1;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_specific");
#endif

  if (H5VL_REQUEST_WAITANY == specific_type ||
    H5VL_REQUEST_WAITSOME == specific_type ||
    H5VL_REQUEST_WAITALL == specific_type) {
    va_list tmp_arguments;
    size_t req_count;

    /* Sanity check */
    assert(obj == NULL);

    /* Get enough info to call the underlying connector */
    va_copy(tmp_arguments, arguments);
    req_count = va_arg(tmp_arguments, size_t);

    /* Can only use a request to invoke the underlying VOL connector when there's >0 requests */
    if (req_count > 0) {
      void** req_array;
      void** under_req_array;
      uint64_t timeout;
      openvisus_vol_t* o;
      size_t u;               /* Local index variable */

      /* Get the request array */
      req_array = va_arg(tmp_arguments, void**);

      /* Get a request to use for determining the underlying VOL connector */
      o = (openvisus_vol_t*)req_array[0];

      /* Create array of underlying VOL requests */
      under_req_array = (void**)malloc(req_count * sizeof(void**));
      for (u = 0; u < req_count; u++)
        under_req_array[u] = ((openvisus_vol_t*)req_array[u])->under;

      /* Remove the timeout value from the vararg list (it's used in all the calls below) */
      timeout = va_arg(tmp_arguments, uint64_t);

      /* Release requests that have completed */
      if (H5VL_REQUEST_WAITANY == specific_type) {
        size_t* idx;          /* Pointer to the index of completed request */
        H5ES_status_t* status;  /* Pointer to the request's status */

        /* Retrieve the remaining arguments */
        idx = va_arg(tmp_arguments, size_t*);
        assert(*idx <= req_count);
        status = va_arg(tmp_arguments, H5ES_status_t*);

        /* Reissue the WAITANY 'request specific' call */
        ret_value = openvisus_vol_request_specific_reissue(o->under, o->under_vol_id, specific_type, req_count, under_req_array, timeout,
          idx,
          status);
      } /* end if */
      else if (H5VL_REQUEST_WAITSOME == specific_type) {
        size_t* outcount;               /* # of completed requests */
        unsigned* array_of_indices;     /* Array of indices for completed requests */
        H5ES_status_t* array_of_statuses; /* Array of statuses for completed requests */

        /* Retrieve the remaining arguments */
        outcount = va_arg(tmp_arguments, size_t*);
        assert(*outcount <= req_count);
        array_of_indices = va_arg(tmp_arguments, unsigned*);
        array_of_statuses = va_arg(tmp_arguments, H5ES_status_t*);

        /* Reissue the WAITSOME 'request specific' call */
        ret_value = openvisus_vol_request_specific_reissue(o->under, o->under_vol_id, specific_type, req_count, under_req_array, timeout, outcount, array_of_indices, array_of_statuses);
      } /* end else-if */
      else {      /* H5VL_REQUEST_WAITALL == specific_type */
        H5ES_status_t* array_of_statuses; /* Array of statuses for completed requests */

        /* Retrieve the remaining arguments */
        array_of_statuses = va_arg(tmp_arguments, H5ES_status_t*);

        /* Reissue the WAITALL 'request specific' call */
        ret_value = openvisus_vol_request_specific_reissue(o->under, o->under_vol_id, specific_type, req_count, under_req_array, timeout, array_of_statuses);
      } /* end else */

      /* Release array of requests for underlying connector */
      free(under_req_array);
    } /* end if */

    /* Finish use of copied vararg list */
    va_end(tmp_arguments);
  } /* end if */
  else {
    openvisus_vol_t* o = (openvisus_vol_t*)obj;

    ret_value = H5VLrequest_specific(o->under, o->under_vol_id, specific_type, arguments);
  } /* end else */

  return ret_value;
} /* end openvisus_vol_request_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_optional
  *
  * Purpose:     Perform a connector-specific operation for a request
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_optional(void* obj, H5VL_request_optional_t opt_type,
    va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_optional");
#endif

  ret_value = H5VLrequest_optional(o->under, o->under_vol_id, opt_type, arguments);

  return ret_value;
} /* end openvisus_vol_request_optional() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_request_free
  *
  * Purpose:     Releases a request, allowing the operation to complete without
  *              application tracking
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *-------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_request_free(void* obj)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_request_free");
#endif

  ret_value = H5VLrequest_free(o->under, o->under_vol_id);

  if (ret_value >= 0)
    openvisus_vol_free_obj(o);

  return ret_value;
} /* end openvisus_vol_request_free() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_blob_put
  *
  * Purpose:     Handles the blob 'put' callback
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_blob_put(void* obj, const void* buf, size_t size,
    void* blob_id, void* ctx)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_blob_put");
#endif

  ret_value = H5VLblob_put(o->under, o->under_vol_id, buf, size,
    blob_id, ctx);

  return ret_value;
} /* end openvisus_vol_blob_put() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_blob_get
  *
  * Purpose:     Handles the blob 'get' callback
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_blob_get(void* obj, const void* blob_id, void* buf,
    size_t size, void* ctx)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_blob_get");
#endif

  ret_value = H5VLblob_get(o->under, o->under_vol_id, blob_id, buf,
    size, ctx);

  return ret_value;
} /* end openvisus_vol_blob_get() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_blob_specific
  *
  * Purpose:     Handles the blob 'specific' callback
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_blob_specific(void* obj, void* blob_id,
    H5VL_blob_specific_t specific_type, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_blob_specific");
#endif

  ret_value = H5VLblob_specific(o->under, o->under_vol_id, blob_id,
    specific_type, arguments);

  return ret_value;
} /* end openvisus_vol_blob_specific() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_blob_optional
  *
  * Purpose:     Handles the blob 'optional' callback
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_blob_optional(void* obj, void* blob_id,
    H5VL_blob_optional_t opt_type, va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_blob_optional");
#endif

  ret_value = H5VLblob_optional(o->under, o->under_vol_id, blob_id,
    opt_type, arguments);

  return ret_value;
} /* end openvisus_vol_blob_optional() */


/*---------------------------------------------------------------------------
  * Function:    openvisus_vol_token_cmp
  *
  * Purpose:     Compare two of the connector's object tokens, setting
  *              *cmp_value, following the same rules as strcmp().
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *---------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_token_cmp(void* obj, const H5O_token_t * token1,
    const H5O_token_t * token2, int* cmp_value)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_token_cmp");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token1);
  assert(token2);
  assert(cmp_value);

  ret_value = H5VLtoken_cmp(o->under, o->under_vol_id, token1, token2, cmp_value);

  return ret_value;
} /* end openvisus_vol_token_cmp() */


/*---------------------------------------------------------------------------
  * Function:    openvisus_vol_token_to_str
  *
  * Purpose:     Serialize the connector's object token into a string.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *---------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_token_to_str(void* obj, H5I_type_t obj_type,
    const H5O_token_t * token, char** token_str)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_token_to_str");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token);
  assert(token_str);

  ret_value = H5VLtoken_to_str(o->under, obj_type, o->under_vol_id, token, token_str);

  return ret_value;
} /* end openvisus_vol_token_to_str() */


/*---------------------------------------------------------------------------
  * Function:    openvisus_vol_token_from_str
  *
  * Purpose:     Deserialize the connector's object token from a string.
  *
  * Return:      Success:    0
  *              Failure:    -1
  *
  *---------------------------------------------------------------------------
  */
static herr_t
  openvisus_vol_token_from_str(void* obj, H5I_type_t obj_type,
    const char* token_str, H5O_token_t * token)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_token_from_str");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token);
  assert(token_str);

  ret_value = H5VLtoken_from_str(o->under, obj_type, o->under_vol_id, token_str, token);

  return ret_value;
} /* end openvisus_vol_token_from_str() */


/*-------------------------------------------------------------------------
  * Function:    openvisus_vol_optional
  *
  * Purpose:     Handles the generic 'optional' callback
  *
  * Return:      SUCCEED / FAIL
  *
  *-------------------------------------------------------------------------
  */
herr_t
  openvisus_vol_optional(void* obj, int op_type, hid_t dxpl_id, void** req,
    va_list arguments)
{
  openvisus_vol_t* o = (openvisus_vol_t*)obj;
  herr_t ret_value;

#if ENABLE_LOGGING
  PrintInfo("openvisus_vol_optional");
#endif

  ret_value = H5VLoptional(o->under, o->under_vol_id, op_type,
    dxpl_id, req, arguments);

  return ret_value;
} /* end openvisus_vol_optional() */



/* OpenVisus VOL connector class struct */
const H5VL_class_t openvisus_vol_g = 
{
    0,                                          /* version      */
    (H5VL_class_value_t)517,                    /* value        */
    "openvisus_vol",                            /* name         */
    0,                                          /* capability flags */
    openvisus_vol_init,                         /* initialize   */
    openvisus_vol_term,                         /* terminate    */

    {                                           /* info_cls */
        sizeof(openvisus_vol_info_t),           /* size    */
        openvisus_vol_info_copy,                /* copy    */
        openvisus_vol_info_cmp,                 /* compare */
        openvisus_vol_info_free,                /* free    */
        openvisus_vol_info_to_str,              /* to_str  */
        openvisus_vol_str_to_info               /* from_str */
    },
    {                                           /* wrap_cls */
        openvisus_vol_get_object,               /* get_object   */
        openvisus_vol_get_wrap_ctx,             /* get_wrap_ctx */
        openvisus_vol_wrap_object,              /* wrap_object  */
        openvisus_vol_unwrap_object,            /* unwrap_object */
        openvisus_vol_free_wrap_ctx             /* free_wrap_ctx */
    },
    {                                           /* attribute_cls */
        openvisus_vol_attr_create,              /* create */
        openvisus_vol_attr_open,                /* open */
        openvisus_vol_attr_read,                /* read */
        openvisus_vol_attr_write,               /* write */
        openvisus_vol_attr_get,                 /* get */
        openvisus_vol_attr_specific,            /* specific */
        openvisus_vol_attr_optional,            /* optional */
        openvisus_vol_attr_close                /* close */
    },
    {                                           /* dataset_cls */
        openvisus_vol_dataset_create,           /* create */
        openvisus_vol_dataset_open,             /* open */
        openvisus_vol_dataset_read,             /* read */
        openvisus_vol_dataset_write,            /* write */
        openvisus_vol_dataset_get,              /* get */
        openvisus_vol_dataset_specific,         /* specific */
        openvisus_vol_dataset_optional,         /* optional */
        openvisus_vol_dataset_close             /* close */
    },
    {                                           /* datatype_cls */
        openvisus_vol_datatype_commit,          /* commit */
        openvisus_vol_datatype_open,            /* open */
        openvisus_vol_datatype_get,             /* get_size */
        openvisus_vol_datatype_specific,        /* specific */
        openvisus_vol_datatype_optional,        /* optional */
        openvisus_vol_datatype_close            /* close */
    },
    {                                           /* file_cls */
        openvisus_vol_file_create,              /* create */
        openvisus_vol_file_open,                /* open */
        openvisus_vol_file_get,                 /* get */
        openvisus_vol_file_specific,            /* specific */
        openvisus_vol_file_optional,            /* optional */
        openvisus_vol_file_close                /* close */
    },
    {                                           /* group_cls */
        openvisus_vol_group_create,             /* create */
        openvisus_vol_group_open,               /* open */
        openvisus_vol_group_get,                /* get */
        openvisus_vol_group_specific,           /* specific */
        openvisus_vol_group_optional,           /* optional */
        openvisus_vol_group_close               /* close */
    },
    {                                           /* link_cls */
        openvisus_vol_link_create,              /* create */
        openvisus_vol_link_copy,                /* copy */
        openvisus_vol_link_move,                /* move */
        openvisus_vol_link_get,                 /* get */
        openvisus_vol_link_specific,            /* specific */
        openvisus_vol_link_optional             /* optional */
    },
    {                                           /* object_cls */
        openvisus_vol_object_open,              /* open */
        openvisus_vol_object_copy,              /* copy */
        openvisus_vol_object_get,               /* get */
        openvisus_vol_object_specific,          /* specific */
        openvisus_vol_object_optional           /* optional */
    },
    {                                           /* introspect_cls */
        openvisus_vol_introspect_get_conn_cls,  /* get_conn_cls */
        openvisus_vol_introspect_opt_query,     /* opt_query */
    },
    {                                           /* request_cls */
        openvisus_vol_request_wait,             /* wait */
        openvisus_vol_request_notify,           /* notify */
        openvisus_vol_request_cancel,           /* cancel */
        openvisus_vol_request_specific,         /* specific */
        openvisus_vol_request_optional,         /* optional */
        openvisus_vol_request_free              /* free */
    },
    {                                           /* blob_cls */
        openvisus_vol_blob_put,                 /* put */
        openvisus_vol_blob_get,                 /* get */
        openvisus_vol_blob_specific,            /* specific */
        openvisus_vol_blob_optional             /* optional */
    },
    {                                           /* token_cls */
        openvisus_vol_token_cmp,                /* cmp */
        openvisus_vol_token_to_str,             /* to_str */
        openvisus_vol_token_from_str              /* from_str */
    },
    openvisus_vol_optional                  /* optional */
};


extern "C" H5PL_type_t H5PLget_plugin_type(void)
{
  return H5PL_TYPE_VOL;
}

extern const H5VL_class_t openvisus_vol_g;

extern "C" const void* H5PLget_plugin_info(void)
{
  return &openvisus_vol_g;
}