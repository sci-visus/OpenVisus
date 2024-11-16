AddExternalLibrary(g2o_types_sim3 
  types_seven_dof_expmap.cpp
  types_seven_dof_expmap.h
)


TARGET_LINK_LIBRARIES(g2o_types_sim3 g2o_types_sba)

set_target_properties(g2o_types_sim3 PROPERTIES FOLDER "ExternalLibs/g2o")