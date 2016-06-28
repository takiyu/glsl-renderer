#include "glsl_classes.h"

using namespace glm;
using namespace std;

/* Shader loaders */
bool readShaderCode(const string& filename, string& dst_code){
	dst_code = "";

	// Open code file
	ifstream code_stream(filename.c_str(), ios::in);
	if(!code_stream.is_open()){
		return false;
	}

	// Read each line
	string line = "";
	while(getline(code_stream, line)){
		dst_code += "\n" + line;
	}
	code_stream.close();

	return true;
}
GLint compileShader(int id, const string& code){
	// Compile Shader
	char const * code_ptr = code.c_str();
	glShaderSource(id, 1, &code_ptr , NULL);
	glCompileShader(id);

	GLint status = GL_FALSE;
	int info_log_length;

	// Check Vertex Shader
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0){
		char mess[info_log_length + 1];
		glGetShaderInfoLog(id, info_log_length, NULL, mess);
		cerr << mess;
	}
	return status;
}
GLuint loadShaders(const string& vs_file, const string& fs_file){
	// Read vertex shader file
	string vs_code;
	if(!readShaderCode(vs_file, vs_code)){
		cerr << "Can't open vertex file (" << vs_file << ")." << endl;
		return 0;
	}

	// Read fragment shader file
	string fs_code;
	if(!readShaderCode(fs_file, fs_code)){
		cerr << "Can't open fragment file (" << fs_file << ")." << endl;
		return 0;
	}

	// Create the shaders
	GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);


	// Compile Vertex Shader
	cout << "Compiling Vertex Shader : " << vs_file << endl;
	if(compileShader(vs_id, vs_code) == GL_FALSE){
		cerr << "Failed to compile Vertex Shader." << endl;
		return 0;
	}
	// Compile Fragment Shader
	cout << "Compiling Fragment Shader : " << fs_file << endl;
	if(compileShader(fs_id, fs_code) == GL_FALSE){
		cerr << "Failed to compile Fragment Shader." << endl;
		return 0;
	}


	// Link the program
	cout << "Linking Program" << endl;
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vs_id);
	glAttachShader(program_id, fs_id);
	glLinkProgram(program_id);

	// Check the program
	GLint status = GL_FALSE;
	int info_log_length;
	glGetProgramiv(program_id, GL_LINK_STATUS, &status);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if(info_log_length > 0){
		char mess[info_log_length + 1];
		glGetProgramInfoLog(program_id, info_log_length, NULL, mess);
		cout << mess;
	}
	if(status == GL_FALSE){
		cerr << "Failed to Ling Program." << endl;
		return 0;
	}

	// Delete Shaders
	glDeleteShader(vs_id);
	glDeleteShader(fs_id);

	return program_id;
}

/* Texture */
TextureRect::TextureRect(int idx, int width, int height, GLenum channel_internalformat, GLenum channel_format, GLenum data_type){
	this->texture_idx = idx;
	this->width = width;
	this->height = height;
	this->internalformat = channel_internalformat;
	this->format = channel_format;
	this->type = data_type;

	/* Create */
	glGenTextures(1, &(this->texture));
	this->active();
	glTexParameterf(GL_TEXTURE_RECTANGLE,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameterf(GL_TEXTURE_RECTANGLE,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
// 	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_RECTANGLE,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_RECTANGLE,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internalformat, width, height, 0, format, type, 0);
}
void TextureRect::active(){
	glActiveTexture(GL_TEXTURE0 + this->texture_idx);
	glBindTexture(GL_TEXTURE_RECTANGLE, this->texture);
}
void TextureRect::bindUniform(GLuint program_id, const string& var_name){
	glUniform1i(glGetUniformLocation(program_id, var_name.c_str()), this->texture_idx);
}
void TextureRect::setBuffer(const GLvoid* data){
	this->active();
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, width, height, format, type, data);
}
void TextureRect::setResizedBuffer(int width, int height, const GLvoid* data){
	this->width = width;
	this->height = height;

	this->active();
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, format, width, height, 0, format, type, data);
}
void TextureRect::copyPixels(int width, int height) {
    this->active();
    glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, internalformat, 0, 0, width, height, 0);
}
