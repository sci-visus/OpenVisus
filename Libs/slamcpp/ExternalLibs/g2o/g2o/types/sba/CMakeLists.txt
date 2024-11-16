AddExternalLibrary(g2o_types_sba 
  types_sba.h     types_six_dof_expmap.h
  types_sba.cpp   types_six_dof_expmap.cpp
  g2o_types_sba_api.h
)


TARGET_LINK_LIBRARIES(g2o_types_sba g2o_core g2o_types_slam3d)

set_target_properties(g2o_types_sba PROPERTIES FOLDER "ExternalLibs/g2o")