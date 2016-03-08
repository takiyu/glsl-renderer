#version 330 core

layout (location = 0) in vec2 src_position;

out vec2 position;

uniform vec2 screen_size;

void main(){
	gl_Position = vec4(src_position, 0, 1);
	position = (src_position+vec2(1,1))*screen_size/2;
}
