#version 430 
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vert_pos;
layout(location = 1) in vec3 vert_color;

layout(location = 0) out vec3 frag_color;

void main() {
  gl_Position = vec4(vert_pos, 0.0, 1.0);
  frag_color = vert_color;
}