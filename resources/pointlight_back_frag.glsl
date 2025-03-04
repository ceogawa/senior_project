#version 330 core
out vec4 FragColor;
in vec2 texCoord;
//in vec3 lightFragPos;
//in vec3 lightFragNor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;
//uniform sampler2D lightMap;
uniform vec3 lightPos;
uniform vec3 lightCol;

void main() {

    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    // removed sampling from lightmap

    vec3 lighting = Albedo * 0.01;
    vec3 lightDir = normalize(lightPos - FragPos);
    float gaussian[] = {0.0625, 0.125, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};
    vec2 offset[] = {vec2(-1, 1), vec2(-1, 0), vec2(-1, -1), vec2(0, 1), vec2(0, 0), vec2(0, -1), vec2(1, 1), vec2(1, 0), vec2(1, -1)};

    float blur_radius = 6.0f;
    float blendedIntensity = 0.0f; 

   // for(int i = 0; i < 9; i++){
        //blendedIntensity += (texture(lightMap, texCoord + (offset[i] * texSize * blur_radius))).r * gaussian[i];
   // }
    //vec3 diffuse = max(dot(Normal, lightDir), 0.0) * blendedIntensity * Albedo;

    float d = length(lightPos - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * lightCol * Albedo;
    if (d != 0){ 
        diffuse = diffuse/(float(d*d));
    }

    lighting += diffuse;
    FragColor = vec4(lighting, 1.0);

} 