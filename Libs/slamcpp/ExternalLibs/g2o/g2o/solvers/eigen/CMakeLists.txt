AddExternalLibrary(g2o_solver_eigen 
  solver_eigen.cpp
  linear_solver_eigen.h
)

TARGET_LINK_LIBRARIES(g2o_solver_eigen g2o_core)

set_target_properties(g2o_solver_eigen PROPERTIES FOLDER "ExternalLibs/g2o")