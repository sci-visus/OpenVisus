AddExternalLibrary(g2o_types_icp 
  types_icp.cpp  types_icp.h
  g2o_types_icp_api.h
)

TARGET_LINK_LIBRARIES(g2o_types_icp g2o_types_sba g2o_types_slam3d)

set_target_properties(g2o_types_icp PROPERTIES FOLDER "ExternalLibs/g2o")