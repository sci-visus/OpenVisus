
execute_process(COMMAND 
	"${PYTHON_EXECUTABLE}" -c "from __future__ import print_function\ntry: import numpy; print(numpy.get_include(), end='')\nexcept:pass\n"
	OUTPUT_VARIABLE __numpy_path)
         
execute_process(COMMAND 
	"${PYTHON_EXECUTABLE}" -c "from __future__ import print_function\ntry: import numpy; print(numpy.__version__, end='')\nexcept:pass\n"
	OUTPUT_VARIABLE __numpy_version__)
	
find_path(NUMPY_INCLUDE_DIR numpy/arrayobject.h HINTS "${__numpy_path}" "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)

if(NUMPY_INCLUDE_DIR)
  set(NUMPY_FOUND 1 CACHE INTERNAL "Python numpy found")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NumPy REQUIRED_VARS NUMPY_INCLUDE_DIR VERSION_VAR __numpy_version__)
