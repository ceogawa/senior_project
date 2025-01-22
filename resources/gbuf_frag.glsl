#version 330 core
out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;

struct Light{
    vec3 Position;
    vec3 Color;
};

const int NR_LIGHTS = 32;
uniform vec3 lightPos[NR_LIGHTS];
//uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;


void main()
{
    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;

    vec3 lighting = Albedo * 0.1;
    vec3 viewDir = normalize(viewPos - FragPos);
    for (int i = 0; i < NR_LIGHTS; ++i){
        vec3 lightDir = normalize(lightPos[i] - FragPos);
        //vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * vec3(0.0f, 0.3f, 0.8f);
        //vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lights[i].Color;
        lighting += diffuse;
    }

     FragColor = vec4(lighting, 1.0);
} 


