#version 330 core
// NOLONGER writing to the accumulation buffer in this shader, but in the norFrag
//layout(location = 0) out vec4 lightAccumulationTexture; // using colorattachment0 now for now

in vec2 texCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec; 
uniform sampler2D lightBuf;

//uniform vec3 lightPos;
//uniform vec3 lightCol;
//float lightRadius;

out vec4 color;

void main() {

// THEN resample depth information and convert back from normalized value
// MUST TWEAK LIGHTING CALCULATIONS, distance attenuation, etc. 

    vec3 FragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    // BASE CODE ADDITION
    // reconstruct the lightNormals/lightDirection from the lightFBO. ***********************
    vec3 lightV = vec3(-1.0) + 2.0*(texture(lightBuf, texCoord).rgb);

    // vector from light center to scene fragment
    // vec3 lightDir = normalize(lightPos - FragPos);

    //float dist = length(lightPos - FragPos);
    float dC = max(0, dot(normalize(lightV), normalize(Normal)));

    // vec3 temporary lightColor
    vec3 diffuse = dC * Albedo * vec3(0.7);

    // TODO CALCULATE SPECULAR LIGHT CONTRIBUTION
    //vec3 specular = 
    //float distance_threshold = 0.2;
    //float distance_threshold = lightRadius - 0.1;
    //if (dist > distance_threshold){ 
       // diffuse = diffuse/(float(dist*dist + 1.0));
    //}

    //lighting = diffuse;
    color = vec4(diffuse, 1.0);

}