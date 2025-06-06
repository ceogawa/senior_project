#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gDepth;

in vec3 fragPos;
in vec3 fragNor;

uniform vec3 MatAmb;
uniform vec3 MatDif;

void main()
{
    // INITIALIZES THE GBUFFER WITH DATA
    // store the fragment position vector in the first gbuffer texture
   // gl_FragCoord.z 

   // TODO we want to store the z coordinate normalized based on the frustum.  
    gPosition = fragPos;
    // also store the per-fragment normals into the gbuffer
    // *************is there a reason why normal for lights is vec4 TODO
    gNormal = 0.5f * (normalize(fragNor) + vec3(1.0));
    //gNormal = vec3(0.5f*(normal+vec3(1.0)));
    //color = vec4(0.5f*(normal+vec3(1.0)), 1.0);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = MatDif;
    
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 0.1;

    // gDepth = vec3(0.5f * (-fragPos.z + vec3(1.0)));
    // linearize depth based on viewspace near and far planes
    float linearDepth = (-fragPos.z - 0.1f)/(100.0f - 0.1f);
    // //  map to ndc?
    // gDepth = vec3(1.0f - vec3(0.5f * (linearDepth + vec3(1.0))));
   
    // float normalizedDepth = 0.5f * (-fragPos.z + 1.0f);
    gDepth = vec3(1.0 - linearDepth);
} 