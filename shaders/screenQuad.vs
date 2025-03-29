#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 0) uniform vec3 offset;

out vec2 TexCoords;
//out vec3 Offset;

void main()
{
    TexCoords = aTexCoords;
    //Offset = offset;
    vec3 pos = aPos + offset;
    gl_Position = vec4(pos.x/pos.z, pos.y/pos.z, pos.z, 1.0);
}
