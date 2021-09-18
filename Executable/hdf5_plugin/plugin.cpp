#include "plugin.h"
#include <H5VLpublic.h>  

#include <Visus/Kernel.h>
#include <Visus/Db.h>
#include <Visus/Path.h>
#include <Visus/StringUtils.h>
#include <Visus/Utils.h>
#include <Visus/DType.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

using namespace Visus;

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

typedef struct
{
  hid_t vol_id;         // VOL ID   for under VOL 
  void* vol_info;       // VOL info for under VOL 
}
PluginVOL;

typedef struct  
{
  hid_t  vol_id;       // VOL ID           for under VOL
  void*  object;       // object           for under VOL
}
PluginObject;

typedef struct 
{
  hid_t vol_id;         // VOL ID           for under VOL
  void* context;       // wrapping context for under VOL 
}
PluginContext;

//predeclaration
extern const H5VL_class_t openvisus_vol_instance;

//////////////////////////////////////////////////////////////////////////////
// Create a new pass through object for an underlying object
static PluginObject* CreatePluginObject(void* under_obj, hid_t vol_id)
{
  PluginObject* ret = (PluginObject*)calloc(1, sizeof(PluginObject));
  ret->object = under_obj;
  ret->vol_id = vol_id;
  H5Iinc_ref(ret->vol_id);
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Release a pass through object
// Note:	Take care to preserve the current HDF5 error stack when calling HDF5 API calls.
static herr_t FreePluginObject(PluginObject* obj)
{
  hid_t err_id = H5Eget_current_stack();
  H5Idec_ref(obj->vol_id);
  H5Eset_current_stack(err_id);
  free(obj);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
static herr_t my_init(hid_t vipl_id)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_init");
#endif

  static int argn = 1;
  static String program_name = Path(Utils::getCurrentApplicationFile()).toString();
  static const char* argv[] = { program_name.c_str() };

  SetCommandLine(argn, argv);

#if VISUS_PYTHON
  EmbeddedPythonInit();
  auto acquire_gil = PyGILState_Ensure();
  PyRun_SimpleString("from OpenVisus import *");
  PyGILState_Release(acquire_gil);
#else
  DbModule::attach();
#endif

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
static herr_t my_term(void)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_term");
#endif

  DbModule::detach();

#if VISUS_PYTHON
  EmbeddedPythonShutdown();
#endif

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
static void* my_info_copy(const void* _info)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_info_copy");
#endif

  const PluginVOL* info = (const PluginVOL*)_info;

  // Allocate new VOL info struct for the pass through connector 
  PluginVOL* new_info = (PluginVOL*)calloc(1, sizeof(PluginVOL));

  // Increment reference count on underlying VOL ID, and copy the VOL info 
  new_info->vol_id = info->vol_id;
  H5Iinc_ref(new_info->vol_id);

  if (info->vol_info)
    H5VLcopy_connector_info(new_info->vol_id, &(new_info->vol_info), info->vol_info);

  return new_info;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_info_cmp(int* cmp_value, const void* _info1, const void* _info2)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_info_cmp");
#endif

  const PluginVOL* info1 = (const PluginVOL*)_info1; VisusReleaseAssert(info1);
  const PluginVOL* info2 = (const PluginVOL*)_info2; VisusReleaseAssert(info2);

  // Initialize comparison value 
  *cmp_value = 0;

  // Compare under VOL connector classes 
  H5VLcmp_connector_cls(cmp_value, info1->vol_id, info2->vol_id);
  if (*cmp_value != 0)
    return 0;

  // Compare under VOL connector info objects 
  H5VLcmp_connector_info(cmp_value, info1->vol_id, info1->vol_info, info2->vol_info);
  if (*cmp_value != 0)
    return 0;

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_info_free(void* _info)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_info_free");
#endif

  // Note:	Take care to preserve the current HDF5 error stack when calling HDF5 API calls.

  PluginVOL* info = (PluginVOL*)_info;
  hid_t err_id = H5Eget_current_stack();

  // Release underlying VOL ID and info
  if (info->vol_info)
    H5VLfree_connector_info(info->vol_id, info->vol_info);
  H5Idec_ref(info->vol_id);

  H5Eset_current_stack(err_id);

  // Free pass through info object itself 
  free(info);

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_info_to_str(const void* _info, char** str)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_info_to_str");
#endif

  const PluginVOL* info = (const PluginVOL*)_info;

  // Get value and string for underlying VOL connector 
  H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
  H5VLget_value(info->vol_id, &under_value);

  char* under_vol_string = nullptr;
  H5VLconnector_info_to_str(info->vol_info, info->vol_id, &under_vol_string);

  // Determine length of underlying VOL info string 
  size_t under_vol_str_len = 0;
  if (under_vol_string)
    under_vol_str_len = strlen(under_vol_string);

  // Allocate space for our info 
  *str = (char*)H5allocate_memory(32 + under_vol_str_len, (hbool_t)0);
  VisusReleaseAssert(*str);

  // Encode our info
  // Normally we'd use snprintf() here for a little extra safety, but that
  // call had problems on Windows until recently. So, to be as platform-independent
  // as we can, we're using sprintf() instead.
   
  sprintf(*str, "under_vol=%u;under_info={%s}", (unsigned)under_value, (under_vol_string ? under_vol_string : ""));

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_info_from_str(const char* str, void** _info)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_info_from_str");
#endif

  // Retrieve the underlying VOL connector value and info 
  unsigned under_vol_value;
  sscanf(str, "under_vol=%u;", &under_vol_value);

  hid_t vol_id = H5VLregister_connector_by_value((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
  const char* under_vol_info_start = strchr(str, '{');
  const char* under_vol_info_end   = strrchr(str, '}');
  VisusReleaseAssert(under_vol_info_end > under_vol_info_start);

  void* vol_info = nullptr;
  if (under_vol_info_end != (under_vol_info_start + 1)) 
  {
    char* under_vol_info_str;

    under_vol_info_str = (char*)malloc((size_t)(under_vol_info_end - under_vol_info_start));
    memcpy(under_vol_info_str, under_vol_info_start + 1, (size_t)((under_vol_info_end - under_vol_info_start) - 1));
    *(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

    H5VLconnector_str_to_info(under_vol_info_str, vol_id, &vol_info);

    free(under_vol_info_str);
  } 

  // Allocate new pass-through VOL connector info and set its fields 
  PluginVOL* info = (PluginVOL*)calloc(1, sizeof(PluginVOL));
  info->vol_id = vol_id;
  info->vol_info = vol_info;

  // Set return value 
  *_info = info;

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
static void* my_get_object(const void* obj)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_get_object");
#endif

  const PluginObject* down = (const PluginObject*)obj;
  return H5VLget_object(down->object, down->vol_id);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_get_wrap_ctx(const void* obj, void** context)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_get_wrap_ctx");
#endif

  const PluginObject* down = (const PluginObject*)obj;

  // Allocate new VOL object wrapping context for the pass through connector
  PluginContext*  down_context = (PluginContext*)calloc(1, sizeof(PluginContext));

  // Increment reference count on underlying VOL ID, 
  down_context->vol_id = down->vol_id;
  H5Iinc_ref(down_context->vol_id);

  // and copy the VOL info
  H5VLget_wrap_ctx(down->object, down->vol_id, /*output*/ &down_context->context);

  // Set wrap context to return 
  *context = down_context;

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_wrap_object(void* obj, H5I_type_t obj_type, void* _down_context)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_wrap_object");
#endif

  PluginContext* down_context = (PluginContext*)_down_context;

  // Wrap the object with the underlying VOL 
  void* under = H5VLwrap_object(obj, obj_type, down_context->vol_id, down_context->context);
  if (!under)
    return nullptr;

  return CreatePluginObject(under, down_context->vol_id);
}


//////////////////////////////////////////////////////////////////////////////
static void* my_unwrap_object(void* obj)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_unwrap_object");
#endif

  PluginObject* down = (PluginObject*)obj;

  // Unrap the object with the underlying VOL 
  void* under = H5VLunwrap_object(down->object, down->vol_id);
  if (!under)
    return nullptr;

  FreePluginObject(down);
  return under;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_free_wrap_ctx(void* _down_context)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_free_wrap_ctx");
#endif

  // Note:	Take care to preserve the current HDF5 error stack when calling HDF5 API calls.

  PluginContext* down_context = (PluginContext*)_down_context;

  hid_t err_id = H5Eget_current_stack();

  // Release underlying VOL ID and wrap context 
  if (down_context->context)
    H5VLfree_wrap_ctx(down_context->context, down_context->vol_id);

  H5Idec_ref(down_context->vol_id);

  H5Eset_current_stack(err_id);

  // Free pass through wrap context object itself 
  free(down_context);

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_attr_create(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name, hid_t type_id, hid_t space_id, hid_t acpl_id,
  hid_t aapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_create");
#endif

  PluginObject* up = (PluginObject*)obj;

  void* under = H5VLattr_create(up->object, loc_params, up->vol_id, name, type_id, space_id, acpl_id, aapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* attr = CreatePluginObject(under, up->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, up->vol_id);

  return (void*)attr;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_attr_open(void* obj, const H5VL_loc_params_t* loc_params, const char* name, hid_t aapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_open");
#endif

  PluginObject* down = (PluginObject*)obj;

  void* under = H5VLattr_open(down->object, loc_params, down->vol_id, name, aapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* attr = CreatePluginObject(under, down->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return (void*)attr;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_read(void* attr, hid_t mem_type_id, void* buf, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_read");
#endif

  PluginObject* down = (PluginObject*)attr;

  herr_t ret_value = H5VLattr_read(down->object, down->vol_id, mem_type_id, buf, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_write(void* attr, hid_t mem_type_id, const void* buf, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_write");
#endif

  PluginObject* down = (PluginObject*)attr;

  herr_t ret_value = H5VLattr_write(down->object, down->vol_id, mem_type_id, buf, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_get(void* obj, H5VL_attr_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_get");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLattr_get(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_specific(void* obj, const H5VL_loc_params_t* loc_params, H5VL_attr_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_specific");
#endif

  PluginObject* down = (PluginObject*)obj;

  herr_t ret_value = H5VLattr_specific(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_optional(void* obj, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLattr_optional(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_attr_close(void* attr, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_attr_close");
#endif

  PluginObject* down = (PluginObject*)attr;

  herr_t ret_value = H5VLattr_close(down->object, down->vol_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  // Release our wrapper, if underlying attribute was closed 
  if (ret_value >= 0)
    FreePluginObject(down);

  return ret_value;
}

//https://github.com/DataLib-ECP/vol-log-based/tree/b13778efd9e0c79135a9d7352104985408078d45

//////////////////////////////////////////////////////////////////////////////
static DType ToDType(hid_t type_id)
{ 
  size_t type_size = H5Tget_size(type_id);
  H5T_class_t type_class = H5Tget_class(type_id);
  H5T_sign_t type_sign = H5Tget_sign(type_id);

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
    switch (type_size) {
    case 4: ret = "float32";
    case 8: ret = "float64";
    default: break;
    }
  }

  VisusReleaseAssert(!ret.empty());
  return DType::fromString(ret);
}


//////////////////////////////////////////////////////////////////////////////
static void* my_dataset_create(
  void* parent_,
  const H5VL_loc_params_t* loc_params,
  const char* name,
  hid_t lcpl_id,
  hid_t type_id,
  hid_t space_id,
  hid_t dcpl_id,
  hid_t dapl_id,
  hid_t dxpl_id,
  void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_create");
#endif

  PluginObject* parent = (PluginObject*)parent_;

  auto dtype = ToDType(type_id);
  
  //shape -> dims
  std::vector<hsize_t> dims(H5S_MAX_RANK);
  int _ndim=H5Sget_simple_extent_dims(space_id, &dims[0], NULL);
  dims.resize(_ndim);
  std::reverse(dims.begin(), dims.end());

  PrintInfo("Creating dataset","name",name,"dtype",dtype,"dims",StringUtils::join(dims));

  // Filters not supported
#if 1
  {
    int nfilter = H5Pget_nfilters(dcpl_id);
    for (int i = 0; i < nfilter; i++)
    {
      unsigned int flags = 0;
      std::vector<unsigned int> cd_values(256); //out
      String name(256, 0);
      unsigned int filter_config = 0; //inout

      size_t cd_number_elements = cd_values.size();
      int name_len = (int)name.size();
      auto filter_id = H5Pget_filter2(dcpl_id, (unsigned int)i, &flags, &cd_number_elements, &cd_values[0], name_len, &name[0], &filter_config);
      name.resize(name_len);
      cd_values.resize(cd_number_elements);

      PrintWarning("filters not supported","name", name, "flags", flags, "#cd_values", cd_values.size(), "filter_config", filter_config);
    }
  }
#endif

  void* dataset = H5VLdataset_create(parent->object, loc_params, parent->vol_id, name, lcpl_id, type_id, space_id, dcpl_id, dapl_id, dxpl_id, req);
  if (!dataset)
    return nullptr;

  PluginObject* ret = CreatePluginObject(dataset, parent->vol_id);

  //todo async
  VisusReleaseAssert(req == nullptr); 

  return ret;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_dataset_open(void* obj, const H5VL_loc_params_t* loc_params, const char* name, hid_t dapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_open");
#endif

  PluginObject* down = (PluginObject*)obj;
  void* under = H5VLdataset_open(down->object, loc_params, down->vol_id, name, dapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* dset = CreatePluginObject(under, down->vol_id);

  VisusReleaseAssert(req == nullptr); //todo async

  return (void*)dset;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_read(void* dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void* buf, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_read");
#endif

  PluginObject* down = (PluginObject*)dset;
  herr_t ret_value = H5VLdataset_read(down->object, down->vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

  VisusReleaseAssert(req == nullptr); //todo async

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_write(void* dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void* buf, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_write");
#endif

  PluginObject* down = (PluginObject*)dset;
  herr_t ret_value = H5VLdataset_write(down->object, down->vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

  VisusReleaseAssert(req == nullptr); //todo async

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_get(void* dset, H5VL_dataset_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_get");
#endif

  PluginObject* down = (PluginObject*)dset;

  herr_t ret_value = H5VLdataset_get(down->object, down->vol_id, args, dxpl_id, req);

  VisusReleaseAssert(req == nullptr); //todo async

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_specific(void* obj, H5VL_dataset_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_specific");
#endif

  PluginObject* down = (PluginObject*)obj;

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  hid_t vol_id = down->vol_id;

  herr_t ret_value = H5VLdataset_specific(down->object, down->vol_id, args, dxpl_id, req);

  VisusReleaseAssert(req == nullptr); //todo async

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_optional(void* obj, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLdataset_optional(down->object, down->vol_id, args, dxpl_id, req);

  VisusReleaseAssert(req == nullptr); //todo async

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_dataset_close(void* dset, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_dataset_close");
#endif

  PluginObject* down = (PluginObject*)dset;
  herr_t ret_value = H5VLdataset_close(down->object, down->vol_id, dxpl_id, req);

  VisusReleaseAssert(req == nullptr); //todo async

  // Release our wrapper, if underlying dataset was closed 
  if (ret_value >= 0)
    FreePluginObject(down);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_datatype_commit(void* obj, const H5VL_loc_params_t* loc_params,
  const char* name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
  hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_commit");
#endif

  PluginObject* down = (PluginObject*)obj;

  void* under = H5VLdatatype_commit(down->object, loc_params, down->vol_id, name, type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* dt = CreatePluginObject(under, down->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return (void*)dt;
}


/////////////////////////////////////////////////////////////////////////////
static void* my_datatype_open(void* obj, const H5VL_loc_params_t* loc_params, const char* name, hid_t tapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_open");
#endif

  PluginObject* down = (PluginObject*)obj;
  void* under = H5VLdatatype_open(down->object, loc_params, down->vol_id, name, tapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* dt = CreatePluginObject(under, down->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return (void*)dt;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_datatype_get(void* dt, H5VL_datatype_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_get");
#endif

  PluginObject* down = (PluginObject*)dt;
  herr_t ret_value = H5VLdatatype_get(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_datatype_specific(void* obj, H5VL_datatype_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_specific");
#endif

  PluginObject* down = (PluginObject*)obj;

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  hid_t vol_id = down->vol_id;

  herr_t ret_value = H5VLdatatype_specific(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_datatype_optional(void* obj, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLdatatype_optional(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_datatype_close(void* dt, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_datatype_close");
#endif

  PluginObject* down = (PluginObject*)dt;
  VisusReleaseAssert(down->object);

  herr_t ret_value = H5VLdatatype_close(down->object, down->vol_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  // Release our wrapper, if underlying datatype was closed 
  if (ret_value >= 0)
    FreePluginObject(down);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_file_create(const char* name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_create");
#endif

  // Get copy of our VOL info from FAPL 
  PluginVOL* info;
  H5Pget_vol_info(fapl_id, (void**)&info);
  if (!info)
    return nullptr;

  // Copy the File Access Property List (FAPL) 
  hid_t under_fapl_id = H5Pcopy(fapl_id);

  // Set the VOL ID and info for the underlying FAPL 
  H5Pset_vol(under_fapl_id, info->vol_id, info->vol_info);

  // Open the file with the underlying VOL connector 
  PluginObject* file=nullptr;
  void* under = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, req);
  if (under) 
  {
    file = CreatePluginObject(under, info->vol_id);

    // Check for async request 
    if (req && *req)
      *req = CreatePluginObject(*req, info->vol_id);
  } 

  // Close underlying FAPL 
  H5Pclose(under_fapl_id);

  // Release copy of our VOL info 
  my_info_free(info);

  return (void*)file;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_file_open(const char* name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_open");
#endif

  // Get copy of our VOL info from FAPL 
  PluginVOL* info;
  H5Pget_vol_info(fapl_id, (void**)&info); 
  if (!info)
    return nullptr;

  // Copy the FAPL 
  hid_t under_fapl_id = H5Pcopy(fapl_id);

  // Set the VOL ID and info for the underlying FAPL 
  H5Pset_vol(under_fapl_id, info->vol_id, info->vol_info);

  // Open the file with the underlying VOL connector 
  PluginObject* file=nullptr;
  void*  under = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, req);
  if (under) {
    file = CreatePluginObject(under, info->vol_id);

    // Check for async request 
    if (req && *req)
      *req = CreatePluginObject(*req, info->vol_id);
  }

  // Close underlying FAPL 
  H5Pclose(under_fapl_id);

  // Release copy of our VOL info 
  my_info_free(info);

  return (void*)file;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_file_get(void* file, H5VL_file_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_get");
#endif

  PluginObject* down = (PluginObject*)file;

  herr_t ret_value = H5VLfile_get(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_file_specific(void* file, H5VL_file_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_specific");
#endif

  PluginObject* down = (PluginObject*)file;
  PluginObject* new_o;
  H5VL_file_specific_args_t my_args;
  H5VL_file_specific_args_t* new_args;
  PluginVOL* info;
  hid_t vol_id = -1;

  // Check for 'is accessible' operation 
  if (args->op_type == H5VL_FILE_IS_ACCESSIBLE) 
  {
    // Make a (shallow) copy of the arguments 
    memcpy(&my_args, args, sizeof(my_args));

    // Set up the new FAPL for the updated arguments 

    // Get copy of our VOL info from FAPL 
    H5Pget_vol_info(args->args.is_accessible.fapl_id, (void**)&info);

    // Make sure we have info about the underlying VOL to be used 
    if (!info)
      return (-1);

    // Keep the correct underlying VOL ID for later 
    vol_id = info->vol_id;

    // Copy the FAPL 
    my_args.args.is_accessible.fapl_id = H5Pcopy(args->args.is_accessible.fapl_id);

    // Set the VOL ID and info for the underlying FAPL 
    H5Pset_vol(my_args.args.is_accessible.fapl_id, info->vol_id, info->vol_info);

    // Set argument pointer to new arguments 
    new_args = &my_args;

    // Set object pointer for operation 
    new_o = nullptr;
  } 
  // Check for 'delete' operation 
  else if (args->op_type == H5VL_FILE_DELETE) 
  {
    // Make a (shallow) copy of the arguments 
    memcpy(&my_args, args, sizeof(my_args));

    // Set up the new FAPL for the updated arguments 

    // Get copy of our VOL info from FAPL 
    H5Pget_vol_info(args->args.del.fapl_id, (void**)&info);

    // Make sure we have info about the underlying VOL to be used 
    if (!info)
      return (-1);

    // Keep the correct underlying VOL ID for later 
    vol_id = info->vol_id;

    // Copy the FAPL 
    my_args.args.del.fapl_id = H5Pcopy(args->args.del.fapl_id);

    // Set the VOL ID and info for the underlying FAPL 
    H5Pset_vol(my_args.args.del.fapl_id, info->vol_id, info->vol_info);

    // Set argument pointer to new arguments 
    new_args = &my_args;

    // Set object pointer for operation 
    new_o = nullptr;
  } 
  else 
  {
    // Keep the correct underlying VOL ID for later 
    vol_id = down->vol_id;

    // Set argument pointer to current arguments 
    new_args = args;

    // Set object pointer for operation 
    new_o = (PluginObject*)down->object;
  } // end else 

  herr_t ret_value = H5VLfile_specific(new_o, vol_id, new_args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  // Check for 'is accessible' operation 
  if (args->op_type == H5VL_FILE_IS_ACCESSIBLE) {
    // Close underlying FAPL 
    H5Pclose(my_args.args.is_accessible.fapl_id);

    // Release copy of our VOL info 
    my_info_free(info);
  } // end else-if 
  // Check for 'delete' operation 
  else if (args->op_type == H5VL_FILE_DELETE) {
    // Close underlying FAPL 
    H5Pclose(my_args.args.del.fapl_id);

    // Release copy of our VOL info 
    my_info_free(info);
  } // end else-if 
  else if (args->op_type == H5VL_FILE_REOPEN) {
    // Wrap reopened file struct pointer, if we reopened one 
    if (ret_value >= 0 && args->args.reopen.file)
      *args->args.reopen.file = CreatePluginObject(*args->args.reopen.file, down->vol_id);
  } // end else 

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_file_optional(void* file, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_optional");
#endif

  PluginObject* down = (PluginObject*)file;
  herr_t ret_value = H5VLfile_optional(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_file_close(void* file, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_file_close");
#endif

  PluginObject* down = (PluginObject*)file;

  herr_t ret_value = H5VLfile_close(down->object, down->vol_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  // Release our wrapper, if underlying file was closed 
  if (ret_value >= 0)
    FreePluginObject(down);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_group_create(void* obj, const H5VL_loc_params_t* loc_params, const char* name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void** req)
{

#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_create");
#endif

  PluginObject* up = (PluginObject*)obj;

  void* under = H5VLgroup_create(up->object, loc_params, up->vol_id, name, lcpl_id, gcpl_id, gapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* group = CreatePluginObject(under, up->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, up->vol_id);

  return (void*)group;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_group_open(void* obj, const H5VL_loc_params_t* loc_params, const char* name, hid_t gapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_open");
#endif

  PluginObject* down = (PluginObject*)obj;
  void* under = H5VLgroup_open(down->object, loc_params, down->vol_id, name, gapl_id, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject* group = CreatePluginObject(under, down->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return (void*)group;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_group_get(void* obj, H5VL_group_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_get");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLgroup_get(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_group_specific(void* obj, H5VL_group_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_specific");
#endif

  PluginObject* down = (PluginObject*)obj;
  
  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  hid_t vol_id = down->vol_id;

  // Unpack arguments to get at the child file pointer when mounting a file 
  H5VL_group_specific_args_t my_args;
  H5VL_group_specific_args_t* new_args=args;
  if (args->op_type == H5VL_GROUP_MOUNT) {

    // Make a (shallow) copy of the arguments 
    memcpy(&my_args, args, sizeof(my_args));

    // Set the object for the child file 
    my_args.args.mount.child_file = ((PluginObject*)args->args.mount.child_file)->object;

    // Point to modified arguments 
    new_args = &my_args;
  }

  herr_t ret_value = H5VLgroup_specific(down->object, vol_id, new_args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_group_optional(void* obj, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLgroup_optional(down->object, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_group_close(void* grp, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_group_close");
#endif

  PluginObject* down = (PluginObject*)grp;
  herr_t ret_value = H5VLgroup_close(down->object, down->vol_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  // Release our wrapper, if underlying file was closed 
  if (ret_value >= 0)
    FreePluginObject(down);

  return ret_value;
}

/////////////////////////////////////////////////////////////////////////////
static herr_t my_link_create(H5VL_link_create_args_t* args, void* obj, const H5VL_loc_params_t* loc_params, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_link_create");
#endif

  PluginObject* up = (PluginObject*)obj;

  // Try to retrieve the "under" VOL id 
  hid_t vol_id = up ? up->vol_id : -1;

  // Fix up the link target object for hard link creation 
  H5VL_link_create_args_t* new_args = args;
  H5VL_link_create_args_t my_args;
  if (H5VL_LINK_CREATE_HARD == args->op_type) 
  {
    // If it's a non-null pointer, find the 'under object' and re-set the args 
    if (args->args.hard.curr_obj) 
    {
      // Make a (shallow) copy of the arguments 
      memcpy(&my_args, args, sizeof(my_args));

      // Check if we still need the "under" VOL ID 
      if (vol_id < 0)
        vol_id = ((PluginObject*)args->args.hard.curr_obj)->vol_id;

      // Set the object for the link target 
      my_args.args.hard.curr_obj = ((PluginObject*)args->args.hard.curr_obj)->object;

      // Set argument pointer to modified parameters 
      new_args = &my_args;
    }
  }

  // Re-issue 'link create' call, possibly using the unwrapped pieces 
  herr_t ret_value = H5VLlink_create(new_args, (up ? up->object : nullptr), loc_params, vol_id, lcpl_id, lapl_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_link_copy(
  void* src_obj, const H5VL_loc_params_t* loc_params1,
  void* dst_obj, const H5VL_loc_params_t* loc_params2, 
  hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_link_copy");
#endif

  // Renames an object within an HDF5 container and copies it to a new group.  
  // The original name SRC is unlinked from the group graph and then inserted with the new name DST (which can specify a new path for the object) as an atomic operation. 
  // The names are interpreted relative to SRC_LOC_ID and DST_LOC_ID, which are either file IDs or group ID.

  PluginObject* o_src = (PluginObject*)src_obj;
  PluginObject* o_dst = (PluginObject*)dst_obj;

  // Retrieve the "under" VOL id 
  hid_t vol_id = o_src? o_src->vol_id : (o_dst ? o_dst->vol_id  : -1);
  VisusReleaseAssert(vol_id > 0);

  herr_t ret_value = H5VLlink_copy(
    (o_src ? o_src->object : nullptr), 
    loc_params1, 
    (o_dst ? o_dst->object : nullptr), 
    loc_params2, 
    vol_id, 
    lcpl_id, 
    lapl_id, 
    dxpl_id, 
    req
  );

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


////////////////////////////////////////////////////////////////////////////// 
static herr_t my_link_move(void* src_obj, const H5VL_loc_params_t* loc_params1, void* dst_obj, const H5VL_loc_params_t* loc_params2, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_link_move");
#endif

  // The original name SRC is unlinked from the group graph and then inserted with the new name DST (which can specify a new path for the object) as an atomic operation. 
  // The namesare interpreted relative to SRC_LOC_ID and DST_LOC_ID, which are either file IDs or group ID.

  PluginObject* o_src = (PluginObject*)src_obj;
  PluginObject* o_dst = (PluginObject*)dst_obj;

  // Retrieve the "under" VOL id 
  hid_t vol_id = o_src? o_src->vol_id  : (o_dst ? o_dst->vol_id  : -1); VisusReleaseAssert(vol_id > 0);

  herr_t ret_value = H5VLlink_move(
    (o_src ? o_src->object : nullptr), 
    loc_params1, 
    (o_dst ? o_dst->object : nullptr), 
    loc_params2, 
    vol_id, 
    lcpl_id, 
    lapl_id, 
    dxpl_id, 
    req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_link_get(void* obj, const H5VL_loc_params_t* loc_params, H5VL_link_get_args_t* args, hid_t dxpl_id, void** req)
{
  PluginObject* down = (PluginObject*)obj;

#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_link_get");
#endif

  herr_t ret_value = H5VLlink_get(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}

//////////////////////////////////////////////////////////////////////////////
static herr_t my_link_specific(void* obj, const H5VL_loc_params_t* loc_params, H5VL_link_specific_args_t* args, hid_t dxpl_id, void** req)
{

#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_link_specific");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLlink_specific(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_link_optional(void* obj, const H5VL_loc_params_t* loc_params, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("LINK Optional");
#endif

  PluginObject* down = (PluginObject*)obj;

  herr_t ret_value = H5VLlink_optional(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static void* my_object_open(void* obj, const H5VL_loc_params_t* loc_params, H5I_type_t* opened_type, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_object_open");
#endif

  PluginObject* down = (PluginObject*)obj;

  void*  under = H5VLobject_open(down->object, loc_params, down->vol_id, opened_type, dxpl_id, req);
  if (!under)
    return nullptr;

  PluginObject*  new_obj = CreatePluginObject(under, down->vol_id);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return (void*)new_obj;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_object_copy(void* src_obj, const H5VL_loc_params_t* src_loc_params,
  const char* src_name, void* dst_obj, const H5VL_loc_params_t* dst_loc_params,
  const char* dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id,
  void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_object_copy");
#endif

  PluginObject* o_src = (PluginObject*)src_obj;
  PluginObject* o_dst = (PluginObject*)dst_obj;

  herr_t ret_value = H5VLobject_copy(o_src->object, src_loc_params, src_name, o_dst->object, dst_loc_params, dst_name, o_src->vol_id, ocpypl_id, lcpl_id, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, o_src->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_object_get(void* obj, const H5VL_loc_params_t* loc_params, H5VL_object_get_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_object_get");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLobject_get(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}

//////////////////////////////////////////////////////////////////////////////
static herr_t my_object_specific(void* obj, const H5VL_loc_params_t* loc_params,H5VL_object_specific_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_object_specific");
#endif

  PluginObject* down = (PluginObject*)obj;

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  hid_t vol_id = down->vol_id;

  herr_t ret_value = H5VLobject_specific(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_object_optional(void* obj, const H5VL_loc_params_t* loc_params, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_object_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLobject_optional(down->object, loc_params, down->vol_id, args, dxpl_id, req);

  // Check for async request 
  if (req && *req)
    *req = CreatePluginObject(*req, down->vol_id);

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_introspect_get_conn_cls(void* obj, H5VL_get_conn_lvl_t lvl, const H5VL_class_t** conn_cls)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_introspect_get_conn_cls");
#endif
  
  if (H5VL_GET_CONN_LVL_CURR == lvl)
  {
    *conn_cls = &openvisus_vol_instance;
    return 0;
  }
  else
  {
    PluginObject* down = (PluginObject*)obj;
    return H5VLintrospect_get_conn_cls(down->object, down->vol_id, lvl, conn_cls);
  }
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_introspect_get_cap_flags(const void* _info, unsigned* cap_flags)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_introspect_get_cap_flags");
#endif

  const PluginVOL* info = (const PluginVOL*)_info;
  herr_t ret_value = H5VLintrospect_get_cap_flags(info->vol_info, info->vol_id, cap_flags);

  if (ret_value >= 0)
    *cap_flags |= openvisus_vol_instance.cap_flags;

  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_introspect_opt_query(void* obj, H5VL_subclass_t cls,
  int op_type, uint64_t* flags)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_introspect_opt_query");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLintrospect_opt_query(down->object, down->vol_id, cls, op_type, flags);
  return ret_value;
}

//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_wait(void* obj, uint64_t timeout, H5VL_request_status_t* status)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_wait");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLrequest_wait(down->object, down->vol_id, timeout, status);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_notify(void* obj, H5VL_request_notify_t cb, void* ctx)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_notify");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLrequest_notify(down->object, down->vol_id, cb, ctx);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_cancel(void* obj, H5VL_request_status_t* status)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_cancel");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLrequest_cancel(down->object, down->vol_id, status);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_specific(void* obj, H5VL_request_specific_args_t* args)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_specific");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLrequest_specific(down->object, down->vol_id, args);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_optional(void* obj, H5VL_optional_args_t* args)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLrequest_optional(down->object, down->vol_id, args);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_request_free(void* obj)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_request_free");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLrequest_free(down->object, down->vol_id);
  if (ret_value >= 0) FreePluginObject(down);
  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_blob_put(void* obj, const void* buf, size_t size, void* blob_id, void* ctx)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_blob_put");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLblob_put(down->object, down->vol_id, buf, size, blob_id, ctx);
}

//////////////////////////////////////////////////////////////////////////////
herr_t my_blob_get(void* obj, const void* blob_id, void* buf, size_t size, void* ctx)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_blob_get");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLblob_get(down->object, down->vol_id, blob_id, buf, size, ctx);
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_blob_specific(void* obj, void* blob_id, H5VL_blob_specific_args_t* args)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_blob_specific");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLblob_specific(down->object, down->vol_id, blob_id, args);
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_blob_optional(void* obj, void* blob_id, H5VL_optional_args_t* args)
{
  PluginObject* down = (PluginObject*)obj;

#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_blob_optional");
#endif

  return H5VLblob_optional(down->object, down->vol_id, blob_id, args);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_token_cmp(void* obj, const H5O_token_t* token1, const H5O_token_t* token2, int* cmp_value)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_token_cmp");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLtoken_cmp(down->object, down->vol_id, token1, token2, cmp_value);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_token_to_str(void* obj, H5I_type_t obj_type, const H5O_token_t* token, char** token_str)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_token_to_str");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLtoken_to_str(down->object, obj_type, down->vol_id, token, token_str);
}


//////////////////////////////////////////////////////////////////////////////
static herr_t my_token_from_str(void* obj, H5I_type_t obj_type, const char* token_str, H5O_token_t* token)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_token_from_str");
#endif

  PluginObject* down = (PluginObject*)obj;
  return H5VLtoken_from_str(down->object, obj_type, down->vol_id, token_str, token);
}


//////////////////////////////////////////////////////////////////////////////
herr_t my_optional(void* obj, H5VL_optional_args_t* args, hid_t dxpl_id, void** req)
{
#ifdef ENABLE_EXT_PASSTHRU_LOGGING
  PrintInfo("my_optional");
#endif

  PluginObject* down = (PluginObject*)obj;
  herr_t ret_value = H5VLoptional(down->object, down->vol_id, args, dxpl_id, req);
  return ret_value;
}


//////////////////////////////////////////////////////////////////////////////
//see https://support.hdfgroup.org/HDF5/doc1.6/RM_H5F.html
const H5VL_class_t openvisus_vol_instance =
{
    H5VL_VERSION,                    // VOL class struct version
    (H5VL_class_value_t)517,         // VOL connector ID
    "openvisus_vol",                 // name
    0,                               // Private characteristics of the pass-through VOL connector
    0,                               // capability flags
    my_init,                         // initialize - Initialize this VOL connector, performing any necessary operations for the connector that will apply to all containers accessed with the connector
    my_term,                         // terminate  - Terminate this VOL connector, performing any necessary operations for the connector that release connector-wide resources (usually created / initialized with the 'init' callback

    // H5VL_info_class_t (DON'T THINK I NEED THIS)
    {                                           
        sizeof(PluginVOL),           // size   
        my_info_copy,                // copy     - Duplicate the connector's info object  
        my_info_cmp,                 // compare  - Compare two of the connector's info objects, setting *cmp_value, following the same rules as strcmp().
        my_info_free,                // free     - Release an info object for the connector.
        my_info_to_str,              // to_str   - Serialize an info object for this connector into a string
        my_info_from_str             // from_str - Deserialize a string into an info object for this connector
    },

    // H5VL_wrap_class_t (DON'T THINK I NEED THIS)
    {                                           
        my_get_object,               // get_object      - Retrieve the 'data' for a VOL object
        my_get_wrap_ctx,             // get_wrap_ctx    - Retrieve a "wrapper context" for an object
        my_wrap_object,              // wrap_object     - Use a "wrapper context" to wrap a data object
        my_unwrap_object,            // unwrap_object   - Unwrap a wrapped object, discarding the wrapper, but returning underlying object.
        my_free_wrap_ctx             // free_wrap_ctx   - Release a "wrapper context" for an object
    },

    // H5VL_attr_class_t (DON'T THINK I NEED THIS)
    {                                           
        my_attr_create,              // create     - Creates an attribute on an object
        my_attr_open,                // open       - Opens an attribute on an object
        my_attr_read,                // read       - Reads data from attribute
        my_attr_write,               // write      - Writes data to attribute
        my_attr_get,                 // get        - Gets information about an attribute
        my_attr_specific,            // specific   - Specific operation on attribute
        my_attr_optional,            // optional   - Perform a connector-specific operation on an attribute
        my_attr_close                // close      - Closes an attribute
    },

    // H5VL_dataset_class_t (!!!!!!PROBABLY THIS IS *.IDX and *.PIDX!!!!!)
    {                                           
        my_dataset_create,           // create    - Creates a dataset in a container
        my_dataset_open,             // open      - Opens a dataset in a container
        my_dataset_read,             // read      - Reads data elements from a dataset into a buffer
        my_dataset_write,            // write     - Writes data elements from a buffer into a dataset
        my_dataset_get,              // get       - Gets information about a dataset
        my_dataset_specific,         // specific  - Specific operation on a dataset
        my_dataset_optional,         // optional  - Perform a connector-specific operation on a dataset
        my_dataset_close             // close     - Closes a dataset
    },

    // H5VL_datatype_class_t (DON'T THINK I NEED THIS)
    {                                           
        my_datatype_commit,          // commit      - Commits a datatype inside a container
        my_datatype_open,            // open        - Opens a named datatype inside a container.
        my_datatype_get,             // get_size    - Get information about a datatype
        my_datatype_specific,        // specific    - Specific operations for datatypes
        my_datatype_optional,        // optional    - Perform a connector-specific operation on a datatype
        my_datatype_close            // close       - Closes a datatype
    },

    // H5VL_file_class_t  (DON'T THINK I NEED THIS)
    {                                           
        my_file_create,              // create     - Creates a container using this connector
        my_file_open,                // open       - Opens a container created with this connector
        my_file_get,                 // get        - Get info about a file
        my_file_specific,            // specific   - Specific operation on file
        my_file_optional,            // optional   - Perform a connector-specific operation on a file
        my_file_close                // close      - Closes a file.
    },

    // H5VL_group_class_t (DON'T THINK I NEED THIS)
    {                                           
        my_group_create,             // create     - Creates a group inside a container
        my_group_open,               // open       - Opens a group inside a container
        my_group_get,                // get        - Get info about a group
        my_group_specific,           // specific   - Specific operation on a group
        my_group_optional,           // optional   - Perform a connector-specific operation on a group
        my_group_close               // close      - Closes a group.
    },

   // H5VL_link_class_t  (DON'T THINK I NEED THIS)
    {                                           
        my_link_create,              // create    - Creates a hard / soft / UD / external link
        my_link_copy,                // copy      - COpy a link
        my_link_move,                // move      - Moves a link within an HDF5 file to a new group. 
        my_link_get,                 // get       - Get info about a link
        my_link_specific,            // specific  - Specific operation on a link
        my_link_optional             // optional  - Perform a connector-specific operation on a link
    },

    // H5VL_object_class_t (DON'T THINK I NEED THIS)
    // objects include datasets, groups, and committed datatypes. 
    {                                           
        my_object_open,              // open       - Opens an object inside a container.
        my_object_copy,              // copy       - Copies an object inside a container.
        my_object_get,               // get        - Get info about an object
        my_object_specific,          // specific   - Specific operation on an object
        my_object_optional           // optional   - Perform a connector-specific operation for an object
    },

  // H5VL_introspect_class_t (DON'T THINK I NEED THIS)
    {                                          
        my_introspect_get_conn_cls,  // get_conn_cls    - Query the connector class.
        my_introspect_get_cap_flags, // get_cap_flags   - Query the capability flags for this connector and any underlying connector(s).
        my_introspect_opt_query,     // opt_query       - Query if an optional operation is supported by this connector
    },

    // H5VL_request_class_t (I think this is async-related, DON'T THINK I NEED THIS TODO?)
    {                                           
        my_request_wait,             // wait      - Wait (with a timeout) for an async operation to complete
        my_request_notify,           // notify    - Registers a user callback to be invoked when an asynchronous operation completes
        my_request_cancel,           // cancel    - Cancels an asynchronous operation
        my_request_specific,         // specific  - Specific operation on a request
        my_request_optional,         // optional  - Perform a connector-specific operation for a request
        my_request_free              // free      - Releases a request, allowing the operation to complete without application tracking
    },

    // H5VL_blob_class_t (DON'T THINK I NEED THIS)
    // raw binary data
    {                                           
        my_blob_put,                 // put       - Handles the blob 'put' callback
        my_blob_get,                 // get       - Handles the blob 'get' callback
        my_blob_specific,            // specific  - Handles the blob 'specific' callback
        my_blob_optional             // optional  - Handles the blob 'optional' callback
    },

    // H5VL_token_class_t (DON'T THINK I NEED THIS)
    /*
      Introduced new H5O_token_t "object token" type, which represents a
      unique and permanent identifier for referencing an HDF5 object within
      a container; these "object tokens" are meant to replace object addresses
    */
    {                                           
        my_token_cmp,                // cmp      - Compare two of the connector's object tokens, setting *cmp_value, following the same rules as strcmp()
        my_token_to_str,             // to_str   - Serialize the connector's object token into a string.
        my_token_from_str            // from_str - Deserialize the connector's object token from a string.
    },

    my_optional  // Catch-all - Handles the generic 'optional' callback               
};


extern "C" H5PL_type_t H5PLget_plugin_type(void)
{ 
  return H5PL_TYPE_VOL; 
}

extern "C" const void* H5PLget_plugin_info(void)
{ 
  return &openvisus_vol_instance;
}

