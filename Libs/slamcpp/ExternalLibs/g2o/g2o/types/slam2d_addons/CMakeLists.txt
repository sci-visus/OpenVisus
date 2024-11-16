AddExternalLibrary(g2o_types_slam2d_addons 
  line_2d.h
  types_slam2d_addons.cpp types_slam2d_addons.h
  vertex_segment2d.cpp         vertex_segment2d.h
  edge_se2_segment2d.cpp       edge_se2_segment2d.h
  edge_se2_segment2d_line.cpp       edge_se2_segment2d_line.h
  edge_se2_segment2d_pointLine.cpp       edge_se2_segment2d_pointLine.h
  vertex_line2d.cpp            vertex_line2d.h
  edge_se2_line2d.cpp          edge_se2_line2d.h
  edge_line2d_pointxy.cpp          edge_line2d_pointxy.h
  edge_line2d.cpp          edge_line2d.h
  g2o_types_slam2d_addons_api.h
)


TARGET_LINK_LIBRARIES(g2o_types_slam2d_addons g2o_types_slam2d g2o_core)

set_target_properties(g2o_types_slam2d_addons PROPERTIES FOLDER "ExternalLibs/g2o")