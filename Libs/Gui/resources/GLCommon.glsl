#if VISUS_OPENGL_ES
precision highp float;
#endif


#ifndef CLIPPINGBOX_ENABLED
#define CLIPPINGBOX_ENABLED 0
#endif

#if CLIPPINGBOX_ENABLED

  #define CLIPPINGBOX_GLOBALS()\
    uniform vec4       u_clippingbox_plane   [6];\
    varying float      v_clippingbox_distance[6];\
  /*--*/
  
  #define CLIPPINGBOX_VERTEX_SHADER()\
    v_clippingbox_distance[0] = dot(eye_pos, u_clippingbox_plane[0]);\
    v_clippingbox_distance[1] = dot(eye_pos, u_clippingbox_plane[1]);\
    v_clippingbox_distance[2] = dot(eye_pos, u_clippingbox_plane[2]);\
    v_clippingbox_distance[3] = dot(eye_pos, u_clippingbox_plane[3]);\
    v_clippingbox_distance[4] = dot(eye_pos, u_clippingbox_plane[4]);\
    v_clippingbox_distance[5] = dot(eye_pos, u_clippingbox_plane[5]);\
    /*--*/ 
  
  #define CLIPPINGBOX_FRAGMENT_SHADER()\
    if (v_clippingbox_distance[0]<0.0) discard;\
    if (v_clippingbox_distance[1]<0.0) discard;\
    if (v_clippingbox_distance[2]<0.0) discard;\
    if (v_clippingbox_distance[3]<0.0) discard;\
    if (v_clippingbox_distance[4]<0.0) discard;\
    if (v_clippingbox_distance[5]<0.0) discard;\
    /*--*/

#else

  #define CLIPPINGBOX_GLOBALS()
  #define CLIPPINGBOX_VERTEX_SHADER()
  #define CLIPPINGBOX_FRAGMENT_SHADER()

#endif
