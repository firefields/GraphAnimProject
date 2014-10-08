#version 150

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

out vec2 texCoord;
out vec3 fN;
out vec3 fE;
out vec3 fL1, fL2;
out float distModifier1, distModifier2;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;

void main()
{
    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * vPosition).xyz;

    // The vector to the light from the vertex    
    vec3 Lvec1 = LightPosition1.xyz - pos;

    //Vector from center to the light
    vec3 Lvec2 = LightPosition2.xyz;

    // Define direction vectors for Blinn-Phong shading calculation
    fL1 = Lvec1;   // Direction to the light source
    fL2 = Lvec2;
    fE = -pos;     // Direction to the eye/camera

    
    // Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    fN = (ModelView*vec4(vNormal, 0.0)).xyz;
    
    // Determine the distance modifier. Uses inverse proportionality as would be found in the real world.
    distModifier1 = 1.0 / (1.0 + pow(length(Lvec1),2));

    gl_Position = Projection * ModelView * vPosition;
    texCoord = vTexCoord;
}
