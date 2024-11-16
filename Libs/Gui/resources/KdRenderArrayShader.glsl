#include <Visus/GLCommon.glsl>

uniform mat4           u_modelview_matrix;
uniform mat4           u_projection_matrix;

uniform vec3           u_sampler_dims;
uniform vec4           u_sampler_vs;
uniform vec4           u_sampler_vt;

#if TEXTURE_DIM==3
  #define TEXTURE(texcoord) (u_sampler_vs*texture3D(u_sampler,texcoord)+u_sampler_vt)
  uniform sampler3D    u_sampler;
  varying vec3         v_texcoord;
#else
  #define TEXTURE(texcoord) (u_sampler_vs*texture2D(u_sampler,texcoord)+u_sampler_vt)
  uniform sampler2D    u_sampler;
  varying vec2         v_texcoord;
#endif

#if PALETTE_ENABLED
uniform sampler2D u_palette_sampler;
#define PALETTE(texcoord) texture2D(u_palette_sampler,texcoord)
#endif

CLIPPINGBOX_GLOBALS()

/////////////////////////////////////////////////////////////
#ifdef VERTEX_SHADER

attribute vec4       a_position;

#if TEXTURE_DIM==2
  attribute vec2     a_texcoord;
#endif

#if TEXTURE_DIM==3
  attribute vec3     a_texcoord;
#endif

void main()
{
  vec4 eye_pos= u_modelview_matrix * a_position;
  gl_Position=u_projection_matrix * eye_pos;
  CLIPPINGBOX_VERTEX_SHADER()

  v_texcoord = a_texcoord;
}

#endif //VERTEX_SHADER

/////////////////////////////////////////////////////////////
#ifdef FRAGMENT_SHADER

void main()
{
  CLIPPINGBOX_FRAGMENT_SHADER()

  vec4 frag_color=TEXTURE(v_texcoord);

  #if PALETTE_ENABLED
    #if TEXTURE_NCHANNELS==1
      frag_color=PALETTE(vec2(frag_color.r,0.0));
    #elif TEXTURE_NCHANNELS==2
      frag_color.rgb=PALETTE(vec2(frag_color.r,0.0)).rgb;
      frag_color.a  =PALETTE(vec2(frag_color.a,0.0)).a;
    #elif TEXTURE_NCHANNELS==3
      frag_color.r=PALETTE(vec2(frag_color.r,0.0)).r;
      frag_color.g=PALETTE(vec2(frag_color.g,0.0)).g;
      frag_color.b=PALETTE(vec2(frag_color.b,0.0)).b;
      frag_color.a=1.0;
    #elif TEXTURE_NCHANNELS==4
      frag_color.r=PALETTE(vec2(frag_color.r,0.0)).r;
      frag_color.g=PALETTE(vec2(frag_color.g,0.0)).g;
      frag_color.b=PALETTE(vec2(frag_color.b,0.0)).b;
      frag_color.a=PALETTE(vec2(frag_color.a,0.0)).a;
    #endif 
  #endif 

  #if DISCARD_IF_ZERO_ALPHA
	if (frag_color.a<=0.0) discard;
  #endif

  gl_FragColor=frag_color;
}

#endif //FRAGMENT_SHADER

