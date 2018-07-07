macro(AddSample Name)
	file(GLOB Sources ${Name}/*.h ${Name}/*.cpp)
	add_executable(${Name} ${Sources})
	set_target_properties(${Name} PROPERTIES FOLDER Samples/)
endmacro()

AddSample(simple_query)
target_link_libraries(simple_query PUBLIC VisusIdx VisusDb VisusKernel)

if (VISUS_GUI)
	AddSample(simple_viewer2d)
	target_link_libraries(simple_viewer2d PUBLIC VisusAppKit VisusKernel)
endif()







