AddExternalLibrary(g2o_types_slam2d 
  se2.h
  edge_se2_pointxy_bearing.h  edge_se2_prior.h
  edge_se2.cpp                  edge_se2_pointxy_calib.cpp  types_slam2d.cpp
  edge_se2.h                    edge_se2_pointxy_calib.h    vertex_point_xy.cpp
  edge_se2_pointxy.cpp        vertex_point_xy.h
  edge_se2_pointxy.h          vertex_se2.cpp
  edge_se2_pointxy_bearing.cpp  edge_se2_prior.cpp          vertex_se2.h
  parameter_se2_offset.cpp     parameter_se2_offset.h
  edge_se2_offset.cpp          edge_se2_offset.h
  edge_se2_pointxy_offset.cpp  edge_se2_pointxy_offset.h
  edge_se2_xyprior.cpp edge_se2_xyprior.h
  edge_pointxy.cpp             edge_pointxy.h
  edge_se2_twopointsxy.cpp	edge_se2_twopointsxy.h
  edge_se2_lotsofxy.cpp		edge_se2_lotsofxy.h
  g2o_types_slam2d_api.h
)

TARGET_LINK_LIBRARIES(g2o_types_slam2d g2o_core)

set_target_properties(g2o_types_slam2d PROPERTIES FOLDER "ExternalLibs/g2o")