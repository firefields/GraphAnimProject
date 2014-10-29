//  CITS3003 Graphics and Animation Project: Part 2
//  vStart.glsl
//
//  20939143    James Rosher
//  20947475    Callum Schofield


#version 150

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;
in ivec4 boneIDs;
in  vec4 boneWeights;

out vec2 texCoord;
out vec3 fN;
out vec3 fE;
out vec3 fL1, fL2;
out float distModifier1;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;
uniform mat4 boneTransforms[64];

void main()
{
    
    //Calculate Bone Transform
    /*
    mat4 boneTransform = 0;
    for (int i = 0; i <= 3 ; i++)
    {
        boneTransform = boneTransform + boneWeights[i]*boneTransforms[boneIDs[i]];
    }
    */


    mat4 boneTransform = boneWeights[0] * boneTransforms[boneIDs[0]] + 
			 boneWeights[1] * boneTransforms[boneIDs[1]] + 
			 boneWeights[2] * boneTransforms[boneIDs[2]] + 
	    	 	 boneWeights[3] * boneTransforms[boneIDs[3]];


    //Apply boneTransform to Vertex Position and Vertex Normal
    vec4 vPositionMod = boneTransform * vPosition;
    vec4 vNormalMod = boneTransform * vec4(vNormal, 0.0);
    //vec3 vNormalMod = (boneTransform * vec4(vNormal,0.0).xyz 

    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * vPositionMod).xyz;

    // The vector to the light from the vertex    
    vec3 Lvec1 = LightPosition1.xyz - pos;

    //Vector from center to the light
    vec3 Lvec2 = LightPosition2.xyz;

    // Define direction vectors for Blinn-Phong shading calculation
    fL1 = Lvec1;   // Direction to the light source
    fL2 = Lvec2;
    fE = -pos;     // Direction to the eye/camera

    

    // Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    fN = (ModelView*vec4(vNormalMod)).xyz;
    
    // Determine the distance modifier. Uses inverse proportionality as would be found in the real world.
    distModifier1 = 1.0 / (1.0 + ( 1.0 * pow(length(Lvec1),2)));

    gl_Position = Projection * ModelView * vPositionMod;
    texCoord = vTexCoord;
} 
