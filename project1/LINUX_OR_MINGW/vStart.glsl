#version 150

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

out vec2 texCoord;
out vec4 color;

out vec3 fN;
out vec3 fE;
out vec3 fL;

//uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
//uniform float Shininess;


void main()
{
    
    fN = vNormal;
    fE = vPosition.xyz;
    fL = LightPosition.xyz;

    if( LightPosition.w != 0.0 )
    {
	fL = LightPosition.xyz - vPosition.xyz;
    }
    
    gl_Position = Projection * ModelView * vPosition;
    
    
    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * vPosition).xyz;


    // The vector to the light from the vertex    
    vec3 Lvec = LightPosition.xyz - pos;
    
    //float modifier = 5.0 / length( Lvec);

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector

    // Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    vec3 N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );


    gl_Position = Projection * ModelView * vPosition;
    texCoord = vTexCoord;
}
