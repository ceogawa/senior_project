#version 330 core
out vec4 FragColor;
in vec2 texCoord;
//in vec3 lightFragPos;
//in vec3 lightFragNor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;
uniform sampler2D lightMap;
//uniform sampler2D gDepth;

uniform vec3 lightPos;
//uniform vec3 lightCol;
uniform float lightRadius;

uniform vec2 resolution;

//float lightRadius = 0.5;

void main() {
    // MAC window resolution
    //vec2 ftexCoord = vec2(gl_FragCoord.x/(2*960.0), gl_FragCoord.y/(2*720.0));
    vec2 ftexCoord = vec2(gl_FragCoord.x/(resolution.x), gl_FragCoord.y/(resolution.y));

    vec3 FragPos = texture(gPosition, ftexCoord).rgb;
    vec3 Normal = texture(gNormal, ftexCoord).rgb;
    vec3 Albedo = texture(gColorSpec, ftexCoord).rgb;
    float Spec = texture(gColorSpec, ftexCoord).a;
    vec4 lightMap = texture(lightMap, ftexCoord).rgba;
    //float Depth = texture(gDepth, ftexCoord).r; // retrieve depth for world space frag pos
    // TODO added
    vec3 lightV = vec3(-1.0) + 2.0*lightMap.rgb;
    vec3 normal = vec3(-1.0) + 2.0*Normal.rgb;

    vec3 lighting = Albedo * 0.01;
    vec3 lightDir = normalize(lightPos - FragPos);  // TODO: currently subtracting world space position - texture space gbuffer position?
    //float d = length(lightPos - FragPos);
    //float worldDepth = 1.0 - Depth;
   // vec3 wsFragPos = vec3(FragPos.x, FragPos.y, worldDepth);


   // lightpos is in VIEW SPACE 
    float d = distance(lightPos, FragPos);
    float lightDirUP = dot(vec3(0.0, 1.0, 0.0), normal) * 0.01;
    // TODO added
    //      float dC = max(0, dot(normalize(lightV), normalize(Normal)));

    // no longer using light normals to calculate light direction
    vec3 diffuse = max(dot(normalize(lightDir), normalize(normal)), 0.0) * Albedo; //* lightCol * Albedo;
    // vec3 diffuse;

    if(d <= lightRadius && d != 0){
        //diffuse = vec3(1.0f, 0.0f, 0.0f);
        diffuse = max(dot(normalize(lightDir), normalize(normal)), 0.0) * Albedo * (d/float(lightRadius));
        //diffuse = diffuse * (1/float(d*d));
     }

    // no longer using light normals to calculate light direction
    // if (d > lightRadius){ 
    //     // discard;
    //     // diffuse = diffuse/(float(d*d)-0.2);
    //     // diffuse = vec3(1.0f, 0.5f, 0.5f);
    // //     //diffuse = diffuse/
    // }
    // float d = length(lightDir);
    lighting += diffuse; //+ (lightDirUP * Albedo);
    // visualize distance (larger than light volume radius)
    // lighting = vec3(d*0.5);
    FragColor = vec4(lighting, 1.0);
   // FragColor = vec4(vec3(d / lightRadius), 1.0);


    // added TEMP debugging
    //FragColor = vec4(ftexCoord, 0.0, 1.0);
    //FragColor = vec4(Normal, 1.0);
    //FragColor = vec4(Albedo, 1.0);

}
