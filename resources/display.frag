#version 460 core
in vec2 uv;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    fragColor = texture(uTexture, uv);
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0/2.2));
}