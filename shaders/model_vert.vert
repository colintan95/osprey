#version 430 

layout(location = 0) in vec3 vert_pos;

layout(std140, binding = 0) uniform Matrices {
  mat4 model_mat;
  mat4 view_mat;
  mat4 proj_mat;
};

void main() {
  gl_Position = proj_mat * view_mat * model_mat * vec4(vert_pos, 1.0);
}