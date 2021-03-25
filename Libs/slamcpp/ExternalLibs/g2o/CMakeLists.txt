
IF(WIN32)
	add_definitions(-DEIGEN_DONT_ALIGN_STATICALLY=1)
  ADD_DEFINITIONS(-DWINDOWS)
  ADD_DEFINITIONS(-DNOMINMAX)
  ADD_DEFINITIONS(-D_USE_MATH_DEFINES)
  add_definitions("/EHsc") # exception handling
  add_definitions("/Ob2")	 # Fix issue: https://github.com/RainerKuemmerle/g2o/issues/66 Link error LNK2005 due to duplicated symbols
ELSE()
  ADD_DEFINITIONS(-DUNIX)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../eigen)

ADD_SUBDIRECTORY(g2o)
