#version 330 core
layout(location = 0) out vec4 lightAccumulationTexture; // using colorattachment0 now for now

in vec2 texCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec; 

uniform vec3 lightPos;
uniform vec3 lightCol;
//float lightRadius;

//out vec4 FragColor;

void main() {
// THEN resample depth information and convert back from normalized value

    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;

    vec3 lighting = Albedo * 0.2;
    // vector from light center to scene fragment
    vec3 lightDir = normalize(lightPos - FragPos);

    float dist = length(lightPos - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * lightCol * Albedo;

    float distance_threshold = 0.2;
    //float distance_threshold = lightRadius - 0.1;
    if (dist > distance_threshold){ 
       // diffuse = diffuse/(float(dist*dist + 1.0));
    }

    lighting += diffuse;
    //FragColor = vec4(lighting, 1.0); // instead of setting fragcolor?
    lightAccumulationTexture = vec4(lighting, 1.0); // Write to light accumulation buffer ? CHATGPT

} 