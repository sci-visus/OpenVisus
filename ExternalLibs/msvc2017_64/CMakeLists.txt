
macro(AddImportedLibrary name include_dir debug_lib release_lib)
	add_library(${name} SHARED IMPORTED GLOBAL)
	set_target_properties(${name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/${include_dir}")
	set_property(TARGET ${name} APPEND PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_DEBUG           ${CMAKE_CURRENT_SOURCE_DIR}/debug/lib/${debug_lib})
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_RELEASE         ${CMAKE_CURRENT_SOURCE_DIR}/lib/${release_lib})    
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_RELWITHDEBINFO  ${CMAKE_CURRENT_SOURCE_DIR}/lib/${release_lib}) 
endmacro()

AddImportedLibrary(zlib       include            zlibd.lib      zlib.lib )
AddImportedLibrary(lz4        include            lz4.lib        lz4.lib)
AddImportedLibrary(TinyXML    include            tinyxml.lib    tinyxml.lib)
AddImportedLibrary(FreeImage  include            FreeImaged.lib FreeImage.lib)
AddImportedLibrary(crypto     include/openssl    libeay32.lib   libeay32.lib)
AddImportedLibrary(ssl        include/openssl    ssleay32.lib   ssleay32.lib)
AddImportedLibrary(curl       include/curl       libcurl.lib    libcurl.lib)

