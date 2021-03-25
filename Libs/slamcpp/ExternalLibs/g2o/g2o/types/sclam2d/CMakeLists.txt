AddExternalLibrary(g2o_types_sclam2d 
  edge_se2_sensor_calib.cpp edge_se2_sensor_calib.h
  vertex_odom_differential_params.cpp vertex_odom_differential_params.h
  edge_se2_odom_differential_calib.cpp edge_se2_odom_differential_calib.h
  odometry_measurement.cpp odometry_measurement.h
  types_sclam2d.cpp types_sclam2d.h
  g2o_types_sclam2d_api.h
)


TARGET_LINK_LIBRARIES(g2o_types_sclam2d g2o_types_slam2d g2o_core)

set_target_properties(g2o_types_sclam2d PROPERTIES FOLDER "ExternalLibs/g2o")