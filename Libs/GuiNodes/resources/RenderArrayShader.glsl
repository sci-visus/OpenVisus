#include <Visus/GLCommon.glsl>

uniform mat4           u_modelview_matrix;
uniform mat4           u_projection_matrix;

uniform vec3           u_sampler_dims;
uniform vec3           u_render_scale;
uniform vec4           u_sampler_vs;
uniform vec4           u_sampler_vt;
  
#if TEXTURE_DIM==3
  uniform sampler3D    u_sampler;
  varying vec3         v_texcoord;
  #define TEXTURE(texcoord) (u_sampler_vs*texture3D(u_sampler,texcoord)+u_sampler_vt)
#else
  uniform sampler2D    u_sampler;
  varying vec2         v_texcoord;
  #define TEXTURE(texcoord) (u_sampler_vs*texture2D(u_sampler,texcoord)+u_sampler_vt)
#endif

#if PALETTE_ENABLED
  uniform sampler2D    u_palette_sampler;
  uniform float        u_palette_opacity;
  
  vec4 PALETTE(float value) {
    vec4 ret=texture2D(u_palette_sampler,vec2(value,0));
    ret.a*=u_palette_opacity;
    return ret;
  }    
#endif

#if LIGHTING_ENABLED
  uniform mat3         u_normal_matrix;
  uniform vec4         u_frontmaterial_ambient;
  uniform vec4         u_frontmaterial_diffuse;
  uniform vec4         u_frontmaterial_specular;
  uniform float        u_frontmaterial_shininess;
  uniform vec4         u_backmaterial_ambient;
  uniform vec4         u_backmaterial_diffuse;
  uniform vec4         u_backmaterial_specular;
  uniform float        u_backmaterial_shininess;
  uniform vec4         u_light_position;
  #if TEXTURE_DIM==2
    varying vec3       v_normal;
  #endif
  varying vec3         v_lightdir;
  varying vec3         v_eyevec;
#endif

CLIPPINGBOX_GLOBALS()


/////////////////////////////////////////////////////////////
#ifdef VERTEX_SHADER

attribute vec4       a_position;

#if TEXTURE_DIM==2
  attribute vec2     a_texcoord;
  #if LIGHTING_ENABLED
    attribute vec3   a_normal;
  #endif
#endif

#if TEXTURE_DIM==3
  attribute vec3     a_texcoord;
#endif

void main()
{
  vec4 eye_pos= u_modelview_matrix * a_position;
  gl_Position =  u_projection_matrix * eye_pos;

  CLIPPINGBOX_VERTEX_SHADER()
  
  v_texcoord  = a_texcoord;
  
  //see http://www.ozone3d.net/tutorials/glsl_lighting_phong_p2.php
  #if LIGHTING_ENABLED

    #if TEXTURE_DIM==2
      v_normal = u_normal_matrix * a_normal;
    #endif

  	v_lightdir  = normalize(vec3(u_light_position.xyz - eye_pos.xyz));
  	v_eyevec    = normalize(-eye_pos.xyz);

  #endif 

}
#endif 

/////////////////////////////////////////////////////////////
#ifdef FRAGMENT_SHADER

#if LIGHTING_ENABLED

  //internalComputeLightingColor
  vec4 internalComputeLightingColor(vec3 N)
  {
    vec4 light_color=vec4(0,0,0,1);
  	vec3 L = normalize(v_lightdir);
  	vec3 E = normalize(v_eyevec);
  	float lambertTerm = dot(N,L);
  	if(lambertTerm>=0.0) // && lamberTerm<=1.0
  	{
  	  light_color  = u_frontmaterial_ambient;
  		light_color += u_frontmaterial_diffuse * lambertTerm;
  		vec3 R = reflect(-L, N);
  		float specular = pow( max(dot(R, E), 0.0), u_frontmaterial_shininess);
  		light_color += u_frontmaterial_specular * specular;
  	}
  	else //if (lambertTerm>=-1.0)
  	{
  	  lambertTerm=-1.0*lambertTerm;N=-N;
  	  light_color  = u_backmaterial_ambient;
  		light_color += u_backmaterial_diffuse * lambertTerm;
  	  vec3 R = reflect(-L, N);
  	  float specular = pow( max(dot(R, E), 0.0), u_backmaterial_shininess);
  	  light_color += u_backmaterial_specular * specular;
  	}
    return light_color;
  }
  
  #if TEXTURE_DIM==3
    vec4 computeLightingColor(vec3 psample)
    {
      	vec3 tstep=vec3(1.0/u_sampler_dims.x,1.0/u_sampler_dims.y,1.0/u_sampler_dims.z);
        vec3 N = -vec3(TEXTURE(psample+vec3(tstep.x,0,0)).r - TEXTURE(psample-vec3(tstep.x,0,0)).r,
                       TEXTURE(psample+vec3(0,tstep.y,0)).r - TEXTURE(psample-vec3(0,tstep.y,0)).r,
                       TEXTURE(psample+vec3(0,0,tstep.z)).r - TEXTURE(psample-vec3(0,0,tstep.z)).r);
        if (length(N)>0.0)
          N=normalize(u_normal_matrix * N);
        return internalComputeLightingColor(N);
    }
  #else
    vec4 computeLightingColor(vec2 psample)
    {
      vec3 N = normalize(v_normal);
      return internalComputeLightingColor(N);
    } 
  #endif
    
#endif


////////////////////////////////////////////////////////////////////////////////
void main()
{
  CLIPPINGBOX_FRAGMENT_SHADER()

  vec4 color=TEXTURE(v_texcoord);
  
  #if PALETTE_ENABLED
    #if TEXTURE_NCHANNELS==1
      color=PALETTE(color.r);
    #elif TEXTURE_NCHANNELS==2
      color.rgb=PALETTE(color.r).rgb;
      color.a  =PALETTE(color.a).a;
    #elif TEXTURE_NCHANNELS==3
      color.r=PALETTE(color.r).r;
      color.g=PALETTE(color.g).g;
      color.b=PALETTE(color.b).b;
      color.a=1.0;
    #elif TEXTURE_NCHANNELS==4
      color.r=PALETTE(color.r).r;
      color.g=PALETTE(color.g).g;
      color.b=PALETTE(color.b).b;
      color.a=PALETTE(color.a).a;
    #endif
  #endif

  #if LIGHTING_ENABLED
    vec4 lighting_color = computeLightingColor(v_texcoord);
    color.rgb = color.rgb * lighting_color.rgb; //scrgiorgio: it was color.rgb += color.rgb * lighting_color.rgb; BUT i think it was wrong
  #endif

  vec4 frag_color = color;

  #if DISCARD_IF_ZERO_ALPHA
  	if (color.a<=0.0)
      discard;
  #endif

  gl_FragColor=color;
}

#endif //FRAGMENT_SHADER

