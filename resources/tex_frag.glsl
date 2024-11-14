#version 330 core

in vec2 texCoord;
out vec4 color;
uniform sampler2D texBuf;
//ignored for now
uniform vec3 Ldir;

/* just pass through the texture color we will add to this next lab */
void main(){
   vec3 tColor = texture( texBuf, texCoord ).rgb;
   color = vec4(tColor, 1.0);
   if (abs(tColor.r) > 0.01 || abs(tColor.g) > 0.01)
   	color = vec4(0.9, 0.9, 0.9, 1.0);

}
