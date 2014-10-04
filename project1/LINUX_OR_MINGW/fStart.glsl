#version 150

in  vec4 color;
in  vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
in  vec3 fN;
in  vec3 fL;
in  vec3 fE;
in  vec4 vPosition;

uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
//uniform mat4 Projection;
uniform vec4 LightPosition;
uniform float Shininess;

uniform sampler2D texture;

void
main()
{
    vec3 N = normalize(fN);
    vec3 E = normalize(fE);
    vec3 L = normalize(fL);

    vec3 H = normalize( L + E);
    vec3 ambient = AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse = Kd*DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    vec3  specular = Ks * SpecularProduct;
    
    if( dot(L, N) < 0.0 ) {
	specular = vec3(0.0, 0.0, 0.0);
    } 

    // Make the modifier 5 / distance between point and light source
    float modifier = 5.0 / length( LightPosition.xyz - (ModelView * vPosition).xyz);

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec4 totalLight =  vec4( (globalAmbient + ambient + ( diffuse + specular) ),1.0);

    gl_FragColor = totalLight;// * texture2D( texture, texCoord * 2.0 );
    gl_FragColor.a = 1.0;

}
