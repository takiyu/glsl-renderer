#ifndef GLSL_TEXTURE_H_150331
#define GLSL_TEXTURE_H_150331

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include <GL/glew.h>
#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/* Shader loaders */
GLuint loadShaders(const std::string& vs_file, const std::string& fs_file);

/* Vertex Attribute */
template<typename T> 
void bindVertexAttribute(GLuint attrib_id, GLuint buffer_id, const std::vector<T>& buff, GLenum type, GLenum usage){
	glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
	unsigned char vec_elem_size = sizeof(buff[0]);
	glBufferData(GL_ARRAY_BUFFER, buff.size() * vec_elem_size, &buff[0], usage);
	glVertexAttribPointer(attrib_id, vec_elem_size/sizeof(buff[0][0]), type, GL_FALSE, 0, (void*)0);
}

/* Texture */
class TextureRect {
public:
	TextureRect(int idx, int width, int height, GLenum channel_internalformat, GLenum channel_format, GLenum data_type);
	void active();
	void bindUniform(GLuint program_id, const std::string& var_name);
	void setBuffer(const GLvoid* data);
	void setResizedBuffer(int width, int height, const GLvoid* data);
	void copyPixels(int width, int height);
private:
	GLuint texture;
	int texture_idx;
	int width, height;
	GLenum internalformat, format, type;
};

#endif

