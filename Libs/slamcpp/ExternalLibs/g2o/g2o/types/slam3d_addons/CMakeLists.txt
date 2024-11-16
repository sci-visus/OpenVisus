AddExternalLibrary(g2o_types_slam3d_addons 
  vertex_se3_euler.cpp
  vertex_se3_euler.h
  edge_se3_euler.cpp
  edge_se3_euler.h
  vertex_plane.cpp
  vertex_plane.h
  edge_se3_plane_calib.cpp
  edge_se3_plane_calib.h
  line3d.cpp line3d.h
  vertex_line3d.cpp vertex_line3d.h
  edge_se3_line.cpp edge_se3_line.h
  edge_plane.cpp edge_plane.h
  edge_se3_calib.cpp edge_se3_calib.h
  types_slam3d_addons.cpp
  types_slam3d_addons.h
)

TARGET_LINK_LIBRARIES(g2o_types_slam3d_addons g2o_types_slam3d g2o_core)

set_target_properties(g2o_types_slam3d_addons PROPERTIES FOLDER "ExternalLibs/g2o")