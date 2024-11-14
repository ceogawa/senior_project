#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fragPos;
in vec3 fragNor;

uniform vec3 MatAmb;
uniform vec3 MatDif;

void main()
{

    // store the fragment position vector in the first gbuffer texture
    gPosition = fragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(fragNor);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = MatDif;
    // store specular intensity in gAlbedoSpec's alpha component
	 //constant could be from a texture
    gAlbedoSpec.a = 0.5;
} 
