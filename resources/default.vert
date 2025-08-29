#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

uniform uvec2 uResolution;
uniform float uFocalLength;
uniform vec3 cameraForward;
uniform vec3 cameraUp;
uniform vec3 cameraRight;

out vec2 uv;
out vec3 rayDir;

void main() {
   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
   uv = aUV;
   rayDir = mat3(cameraRight, cameraUp, cameraForward) * normalize(vec3(uResolution*aUV-uResolution*.5, uFocalLength));
}