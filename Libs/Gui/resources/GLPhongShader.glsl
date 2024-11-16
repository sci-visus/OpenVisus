#include <Visus/GLCommon.glsl>

uniform mat4         u_modelview_matrix;
uniform mat4         u_projection_matrix;
uniform vec4         u_color;

#if LIGHTING_ENABLED
  uniform  vec4      u_light_position;
  uniform  mat3      u_normal_matrix;
  uniform  vec4      u_frontmaterial_ambient;
  uniform  vec4      u_frontmaterial_diffuse;
  uniform  vec4      u_frontmaterial_specular;
  uniform  vec4      u_frontmaterial_emission;
  uniform  float     u_frontmaterial_shininess;
  uniform  vec4      u_backmaterial_ambient;
  uniform  vec4      u_backmaterial_diffuse;
  uniform  vec4      u_backmaterial_specular;
  uniform  vec4      u_backmaterial_emission;
  uniform  float     u_backmaterial_shininess;
#endif



#if TEXTURE_ENABLED
  uniform sampler2D  u_sampler;
  uniform int        u_sampler_envmode;
  uniform int        u_sampler_ncomponents;
#endif

#if COLOR_ATTRIBUTE_ENABLED
  varying vec4       v_color;
#endif

#if TEXTURE_ENABLED
  varying vec2       v_texcoord;
#endif


#if LIGHTING_ENABLED
  varying vec3       v_normal;
  varying vec3       v_light_dir;
  varying vec3       v_eye_vec;
#endif

CLIPPINGBOX_GLOBALS()

/////////////////////////////////////////////////////////////
#ifdef VERTEX_SHADER

attribute vec4       a_position;

#if LIGHTING_ENABLED
  attribute vec3     a_normal;
#endif

#if COLOR_ATTRIBUTE_ENABLED
  attribute vec4     a_color;
#endif

#if TEXTURE_ENABLED
  attribute vec2     a_texcoord;
#endif

void main() 
{
  vec4 eye_pos= u_modelview_matrix * a_position;
  gl_Position = u_projection_matrix * eye_pos;	
  CLIPPINGBOX_VERTEX_SHADER()
	
  #if LIGHTING_ENABLED
	{
  	v_normal = u_normal_matrix * a_normal;
  	vec3 vVertex = vec3(u_modelview_matrix * a_position);
  	v_light_dir  = normalize(vec3(u_light_position.xyz - vVertex));
  	v_eye_vec    = normalize(-vVertex);
  }
  #endif

  #if COLOR_ATTRIBUTE_ENABLED
  {	
	  v_color=a_color;
	}
  #endif
	
  #if TEXTURE_ENABLED
    v_texcoord = a_texcoord;
  #endif
}
#endif 


/////////////////////////////////////////////////////////////
#ifdef FRAGMENT_SHADER

void main() 
{
  CLIPPINGBOX_FRAGMENT_SHADER()
  
  vec4 frag_color = u_color;
  
  #if LIGHTING_ENABLED
  {
  	vec3 N = normalize(v_normal   );
  	vec3 L = normalize(v_light_dir);
  	vec3 E = normalize(v_eye_vec  );

  	if(gl_FrontFacing)
  	{
      frag_color = u_frontmaterial_ambient;
  	  //float NdotL = max(dot(N,L),0.0);
  	  float NdotL = abs(dot(N,L));
  	  if (NdotL>0.0)
  	  {
  	    vec3 R = reflect(-L, N);
  	    //float NdotHV = max(0.0,dot(R, E));
  	    float NdotHV = abs(dot(R, E));
        frag_color += u_frontmaterial_diffuse * NdotL;
        frag_color += u_frontmaterial_specular * pow(NdotHV,u_frontmaterial_shininess);
  		}
  	}
  	else
  	{
      frag_color = u_backmaterial_ambient;
  	  //float NdotL = max(dot(-N,L),0.0);
  	  float NdotL = abs(dot(-N,L));
  	  if (NdotL>0.0)
  	  {
        vec3 R = reflect(-L, -N);
        //float NdotHV=max(0.0,dot(R, E));
        float NdotHV=abs(dot(R, E));
        frag_color += u_backmaterial_diffuse * NdotL;
        frag_color += u_backmaterial_specular * pow(NdotHV,u_backmaterial_shininess);
    	}
  	}
  }
  #endif

  #if COLOR_ATTRIBUTE_ENABLED
  {
    frag_color =v_color;
  }
  #endif

  #if TEXTURE_ENABLED
  {
    vec4 tex_color=texture2D(u_sampler, v_texcoord.st);
    
    //GL_REPLACE
    if (u_sampler_envmode == 0x1E01)
      frag_color.rgba = vec4(1.0, 1.0, 1.0, 1.0);
    
    //modulate
    if      (u_sampler_ncomponents==1) { frag_color.rgb *= vec3(tex_color.r, tex_color.r, tex_color.r);                             } //R
    else if (u_sampler_ncomponents==2) { frag_color.rgb *= vec3(tex_color.r, tex_color.r, tex_color.r); frag_color.a *= tex_color.g;} //RG
    else if (u_sampler_ncomponents==3) { frag_color.rgb *= tex_color.rgb;                                                           } //RGB
    else if (u_sampler_ncomponents==4) { frag_color.rgb *= tex_color.rgb;                               frag_color.a *= tex_color.a;} //RGBA

    frag_color = clamp(frag_color,0.0,1.0);
  }
  #endif  
  
  gl_FragColor = frag_color;
}
#endif

