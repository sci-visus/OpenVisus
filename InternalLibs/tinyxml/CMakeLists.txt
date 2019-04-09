DisableAllWarnings()

AddInternalLibrary(TinyXML 
	tinyxml.cpp 
	tinyxmlparser.cpp 
	xmltest.cpp 
	tinyxmlerror.cpp  
	tinystr.cpp
	tinystr.h 
	tinyxml.h
)

target_include_directories(TinyXML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(TinyXML PROPERTIES FOLDER "InternalLibs/")
