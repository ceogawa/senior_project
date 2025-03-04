#version 330 core
out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;
uniform sampler2D lightMap;

const int NUM_LIGHTS = 16;
uniform vec3 camPos;


void main() {

    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    vec3 Light = texture(lightMap, texCoord).rgb; // black and white image
    float depth = gl_FragCoord.z;

    vec3 lighting = Albedo * 0.01;
    vec3 lightDir = normalize(camPos - FragPos);

    float gaussian[] = {0.0625, 0.125, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};
    vec2 offset[] = {vec2(-1, 1), vec2(-1, 0), vec2(-1, -1), vec2(0, 1), vec2(0, 0), vec2(0, -1), vec2(1, 1), vec2(1, 0), vec2(1, -1)};

    //vec3 lightPos = vec3(0.0f, 0.0f, 0.0f);
    //if (Light.g > 0.0){
        //float radius = 3.0f;
        //float depth = 2*(Light.g) - 1;
        //lightPos = vec3(FragPos) + vec3(0, 0, depth);
        //lightDir = normalize(lightPos - FragPos);
    //}

    vec2 texSize = 1.0 / textureSize(lightMap, 0);
    float blur_radius = 6.0f;
    float blendedIntensity = 0.1f;

    for(int i = 0; i < 9; i++){
        blendedIntensity += (texture(lightMap, texCoord + (offset[i] * texSize * blur_radius))).r * gaussian[i];
    }
    
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * blendedIntensity * Albedo ;


    lighting += diffuse;
    FragColor = vec4(lighting, 1.0);


} 