
if (WIN32)
  add_subdirectory(msvc2017_64)
else()
	message( FATAL_ERROR "VISUS_SELF_CONTAINED not supported")
endif()