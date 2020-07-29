#version 430

layout(location = 0) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D tex_sampler;

void main() {
  out_color = texture(tex_sampler, frag_texcoord);
}