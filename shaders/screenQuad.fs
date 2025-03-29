#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
//in vec3 Offset;

uniform sampler2D tex;

void main()
{
    vec3 texCol = texture(tex, TexCoords).rgb;
    //if(TexCoords.x < 0.5-0.5*Offset.x-0.45*(1+Offset.z) || TexCoords.y < 0.5-0.5*Offset.y-0.45*(1+Offset.z)) texCol = vec3(1,1,1);
    //if(TexCoords.x > 0.5-0.5*Offset.x+0.45*(1+Offset.z) || TexCoords.y > 0.5-0.5*Offset.y+0.45*(1+Offset.z)) texCol = vec3(1,1,1);
    FragColor = vec4(texCol, 1.0);
}
