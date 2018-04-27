if you do NOT want to use external libraries coming with your OS (thourght vcpkg, or brew, or apt get)
you can unzip the ExternalLibs.zip file amd OpenVisus will automatically use those.

For example VisusKernel will do:

if (TARGET FreeImage)
	target_link_libraries(VisusKernel PRIVATE FreeImage)
else()
	find_package(FreeImage REQUIRED)
	target_include_directories(VisusKernel PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
	target_link_libraries(VisusKernel      PRIVATE  ${FREEIMAGE_LIBRARIES})
endif()
