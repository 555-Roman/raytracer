#version 330 core

uniform vec3 uColor;

in vec2 uv;

out vec4 FragColor;

void main() {
   FragColor = vec4(uv, 0.0f, 1.0f);
}