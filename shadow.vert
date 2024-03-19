#version 330 core

layout(location=0) in vec3 pos;

// Model View Projection matrix!
// From the light source's perspective.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view  * vec4(pos, 1);
}