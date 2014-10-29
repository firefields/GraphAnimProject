//  CITS3003 Graphics and Animation Project: Part 2
//  fStart.glsl
//
//  20939143    James Rosher
//  20947475    Callum Schofield

#version 150

//in  vec4 color;
in  vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
in  vec3 fN;
in  vec3 fL1, fL2;
in  vec3 fE;
in  float distModifier1;

out vec4 fColor;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;
uniform float Shininess;
uniform float texScale;

void
main()
{
    vec3 N = normalize( fN);
    vec3 E = normalize( fE);
    vec3 L1 = normalize( fL1);
    vec3 L2 = normalize( fL2);

    vec3 H1 = normalize( fL1 + fE );  // Halfway vector
    vec3 H2 = normalize( fL2 + fE ); 

    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;

    float Kd1 = max( dot(L1, N), 0.0 );
    vec3  diffuse1 = Kd1*DiffuseProduct;

    float Kd2 = max( dot(L2, N), 0.0 );
    vec3  diffuse2 = Kd2*DiffuseProduct; 

    float Ks1 = pow( max(dot(N, H1), 0.0), Shininess );
    float Ks2 = pow( max(dot(N, H2), 0.0), Shininess );

    // Initialise both specular components as zero
    vec3 specular1 = vec3(0.0, 0.0, 0.0);
    vec3 specular2 = vec3(0.0, 0.0, 0.0);

     /*
    if(dot(L1, N) > 0.0)
    {
	vec3  specular1 =( vec3(1.0,1.0,1.0) + (Ks1 * SpecularProduct) ) / 2.0;
    }
    if(dot(L2, N) > 0.0)
    {
        vec3  specular2 =( vec3(1.0,1.0,1.0) + (Ks2 * SpecularProduct) ) / 2.0;
    }*/

    // Only calculates the specular if surface faces light
    if(dot(L1, N) > 0.0)
    {
        vec3  specular1 = Ks1 * SpecularProduct;
    }
    if(dot(L2, N) > 0.0)
    {
        vec3  specular2 =Ks2 * SpecularProduct;
    }

    // Set the specular component to tend towards white
    specular1 = ((vec3(1.0,1.0,1.0)+(vec3(1.0,1.0,1.0)-specular1)) * specular1);
    specular2 = ((vec3(1.0,1.0,1.0)+(vec3(1.0,1.0,1.0)-specular2)) * specular2);


    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    vec4 finalColor;
    finalColor.rgb =  globalAmbient  + ( ambient + diffuse1 + specular1) * distModifier1 + diffuse2 + specular2;
    finalColor.a = 1.0;

    fColor = finalColor * texture2D( texture, texCoord * 2.0 * texScale ); 
}
