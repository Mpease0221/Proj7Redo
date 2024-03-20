#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;

out vec3 fixedNorm;
out vec3 fragPos;
out vec4 fragPosSampleSpace;

// Model View Projection matrix!
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// Our light space matrices.
uniform mat4 shadowMatrix;

void main()
{
    gl_Position = projection * view * model * vec4(pos, 1);
    gl_Position = shadowMatrix * vec4(pos, 1);
    fixedNorm = transpose(inverse(mat3(model))) * norm;
    fragPos = vec3(model * vec4(pos, 1));
    fragPosSampleSpace = shadowMatrix * vec4(pos, 1.0);
}