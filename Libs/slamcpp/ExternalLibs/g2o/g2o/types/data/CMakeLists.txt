AddExternalLibrary(g2o_types_data 
  types_data.cpp
  robot_data.cpp robot_data.h
  vertex_tag.cpp vertex_tag.h
  vertex_ellipse.cpp vertex_ellipse.h
  laser_parameters.cpp laser_parameters.h
  raw_laser.cpp raw_laser.h
  robot_laser.cpp robot_laser.h
  data_queue.cpp data_queue.h
  g2o_types_data_api.h
)

TARGET_LINK_LIBRARIES(g2o_types_data g2o_core g2o_types_slam2d)

set_target_properties(g2o_types_data PROPERTIES FOLDER "ExternalLibs/g2o")
