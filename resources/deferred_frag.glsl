#version 330 core
out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;

const int NUM_LIGHTS = 16;
uniform vec3 lightPos[NUM_LIGHTS];
uniform vec3 lightCol[NUM_LIGHTS];
uniform vec3 viewPos;

// TODO pass as light struct
struct Light{
    vec3 position;
    vec3 colors;
    float intensity;
};

void main()
{

    float radius = 20.0;
    
    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;

    vec3 lighting = Albedo * 0.01;
    vec3 viewDir = normalize(viewPos - FragPos);
    for (int i = 1; i < NUM_LIGHTS; ++i){
        // if point is within radius
            // do lighting calculation
        float d = length(lightPos[i] - FragPos);
        if((d < radius) && d != 0){
            vec3 lightDir = normalize(lightPos[i] - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lightCol[i];
            diffuse = diffuse/(d*d);
            lighting += diffuse;
        }
    }

    //   vec3 lightDir = normalize(lightPos[0] - FragPos);
   // vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lightCol[0];
    //diffuse = diffuse/(d*d);
    //lighting += diffuse;

    //lighting *= 0.3;
    FragColor = vec4(lighting, 1.0);
} 