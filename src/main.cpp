#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "tinyobjloader/tiny_obj_loader.h"

#include "fps_counter.h"
#include "glsl_classes.h"
#include "camera.h"
#include "bvh.h"


using namespace glm;
using namespace std;

Camera camera;
int accum_frame = 0;
vector<vec3> prev_pixel_buff;

const string VS_FILE = "../src/simple.vs";
const string FS_FILE = "../src/simple.fs";

string OBJ_FILE = "../data/CornellBox-Sphere.obj"; // default
const int VSYNC_INTERVAL = 0;

const int TRI_TEX_COL = 512;
const int M_ID_TEX_COL = 512;
const int M_TEX_COL = 2;
const int BVH_TEX_COL = 512;

/* Convert float* to vector<T> */
template<typename T> 
void convToVecs(vector<T>& dst_tex, int width, int height,
                float* src_tex, int src_channel, bool y_reverse=false){

	int dst_channel = sizeof(T) / sizeof(float);
	assert(dst_channel <= src_channel);

	dst_tex.resize(width * height);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			// fetch
			T value;
			for(int c = 0; c < dst_channel; c++) {
				value[c] = src_tex[(y * width + x) * src_channel + c];
			}

			// reverse
			if(y_reverse) {
				int y2 = height - y - 1;
				dst_tex[width * y2 + x] = value;
			} else {
				dst_tex[width * y + x] = value;
			}
		}
	}
}
bool loadObjFile(const string& filename, vector<vec3>& triangle_buff,
                 vector<vec3>& normal_buff, vector<vec2>& texcoord_buf,
                 vector<int>& mat_idx_buff, vector<vec3>& material_buff){
	cout << " obj: " << filename << endl;

	string basepath = ".";
	size_t slash_idx = filename.find_last_of('/');
	if(slash_idx != string::npos) {
		basepath = filename.substr(0, slash_idx + 1);
	}
	cout << "material search path: " <<  basepath << endl;

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, filename.c_str(),
	                            basepath.c_str());
	if (!ret) {
		cerr << err << endl;
		return false;
	}

	// Calc obj buffers
	for(int shape_idx = 0; shape_idx < shapes.size(); shape_idx++){
		int tri_count = shapes[shape_idx].mesh.indices.size() / 3;
		for(int tri_idx = 0; tri_idx < tri_count; tri_idx++){
			int v_idxs[3]; //vertices' indices
			vec3 v[3]; //vertices
			vec3 n[3]; //normals
			vec2 t[3]; //texcoord
			for(int i = 0; i < 3; i++){
				v_idxs[i] = shapes[shape_idx].mesh.indices[tri_idx * 3 + i];

				v[i].x = shapes[shape_idx].mesh.positions[v_idxs[i] * 3 + 0];
				v[i].y = shapes[shape_idx].mesh.positions[v_idxs[i] * 3 + 1];
				v[i].z = shapes[shape_idx].mesh.positions[v_idxs[i] * 3 + 2];

				if(shapes[shape_idx].mesh.normals.size() == 0) {
					n[i].x = n[i].y = n[i].z = 0;
				} else {
					n[i].x = shapes[shape_idx].mesh.normals[v_idxs[i] * 3 + 0];
					n[i].y = shapes[shape_idx].mesh.normals[v_idxs[i] * 3 + 1];
					n[i].z = shapes[shape_idx].mesh.normals[v_idxs[i] * 3 + 2];
				}

				if(shapes[shape_idx].mesh.texcoords.size() == 0) {
					t[i].x = t[i].y = 0;
				} else {
					t[i].x = shapes[shape_idx].mesh.texcoords[v_idxs[i] * 2 + 0];
					t[i].y = shapes[shape_idx].mesh.texcoords[v_idxs[i] * 2 + 1];
				}

				// Add to triangle_buff
				triangle_buff.push_back(v[i]);

				// Add to normal_buff
				normal_buff.push_back((n[i] + 1.f) / 2.f);

				// Add to texcoord_buf
				texcoord_buf.push_back(t[i]);
			}
			// Add to mat_idx_buff
			mat_idx_buff.push_back(shapes[shape_idx].mesh.material_ids[tri_idx]);
		}
	}
	for(int mat_idx = 0; mat_idx < materials.size(); mat_idx++){
		vec3 kd, ks;
		kd.x = materials[mat_idx].diffuse[0];
		kd.y = materials[mat_idx].diffuse[1];
		kd.z = materials[mat_idx].diffuse[2];
		ks.x = materials[mat_idx].specular[0];
		ks.y = materials[mat_idx].specular[1];
		ks.z = materials[mat_idx].specular[2];
		// Add to material_buff
		material_buff.push_back(kd);
		material_buff.push_back(ks);
	}
}


/* Convert Vectors */
template<typename T> 
T getMinPoint(const vector<T>& vec){
	T min_point;
	int size = sizeof(vec[0]) / sizeof(vec[0][0]);
	//Init min
	for(int i = 0; i < size; i++){
		min_point[i] =  INFINITY;
	}
	// Get min
	for(int i = 0; i < vec.size(); i++){
		for(int j = 0; j < size; j++){
			if(min_point[j] > vec[i][j]) min_point[j] = vec[i][j];
		}
	}
	return min_point;
}
template<typename T> 
T getMaxPoint(const vector<T>& vec){
	T max_point;
	int size = sizeof(vec[0]) / sizeof(vec[0][0]);
	//Init max
	for(int i = 0; i < size; i++){
		max_point[i] = -INFINITY;
	}
	// Get max
	for(int i = 0; i < vec.size(); i++){
		for(int j = 0; j < size; j++){
			if(max_point[j] < vec[i][j]) max_point[j] = vec[i][j];
		}
	}
	return max_point;
}
template<typename T> 
float getClampScale(vector<T>& vec){

	T max_point = getMaxPoint(vec);
	T min_point = getMinPoint(vec);

	// Max diff
	T diff = max_point - min_point;
	float max_diff = -INFINITY;
	int size = sizeof(vec[0])/sizeof(vec[0][0]);
	for(int i = 0; i < size; i++){
		if(max_diff < diff[i]){
			max_diff = diff[i];
		}
	}
	return 1.0/max_diff;
}
template<typename T> 
void transformVec(vector<T>& vec, float scale, T shift){
	// Scale
	for(int i = 0; i < vec.size(); i++){
		vec[i] = vec[i] + shift;
		vec[i] *= scale;
	}
	return;
}
template<typename T> 
void transformVec(vector<T>& vec, vec3 scale, T shift){
	// Scale
	for(int i = 0; i < vec.size(); i++){
		vec[i] = vec[i] + shift;
		vec[i] *= scale;
	}
	return;
}
template<typename T>
void joinVectors(const vector<T>& src1, const vector<T>& src2, vector<T>& dst,
                 int src1_cols, int src2_cols){
	int rows = src1.size() / src1_cols;
	//check rows
	assert(src1.size() % src1_cols == 0 && src2.size() % src2_cols == 0);
	assert(rows == src2.size() / src2_cols);

	dst.clear();
	for(int row = 0; row < rows; row++){
		for(int col1 = 0; col1 < src1_cols; col1++){
			dst.push_back(src1[row*src1_cols + col1]);
		}
		for(int col2 = 0; col2 < src2_cols; col2++){
			dst.push_back(src2[row*src2_cols + col2]);
		}
	}
}


/* GLFW Callback */
double pre_mouse_x, pre_mouse_y;
bool mouse_left_pussing = false;
int WIDTH = 360, HEIGHT = 240;
void reshapeFunc(GLFWwindow *window, int width, int height){
	WIDTH = width;
	HEIGHT = height;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	prev_pixel_buff.resize(WIDTH * HEIGHT);
	accum_frame = 0;
}
void keyboardFunc(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS || action == GLFW_REPEAT){
		// Move camera
		float mv_x = 0, mv_y = 0, mv_z = 0;
		if(key == GLFW_KEY_K) mv_x += 1;
		else if(key == GLFW_KEY_J) mv_x += -1;
		else if(key == GLFW_KEY_L) mv_y += 1;
		else if(key == GLFW_KEY_H) mv_y += -1;
		else if(key == GLFW_KEY_P) mv_z += 1;
		else if(key == GLFW_KEY_N) mv_z += -1;
		camera.move(mv_x * 0.05, mv_y * 0.05, mv_z * 0.05);
		// Close window
		if(key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);

		accum_frame = 0;
	}
}
void clickFunc(GLFWwindow* window, int button, int action, int mods){
	if(button == GLFW_MOUSE_BUTTON_LEFT){
		if(action == GLFW_PRESS){
			mouse_left_pussing = true;
		} else if(action == GLFW_RELEASE){
			mouse_left_pussing = false;
		}
	}
}
void motionFunc(GLFWwindow* window, double mouse_x, double mouse_y){
	if(mouse_left_pussing){
		// Camera position rotation
		camera.rotateOrbit(0.005 * (pre_mouse_x - mouse_x),
		                   0.005 * (pre_mouse_y - mouse_y));
		accum_frame = 0;
	}

	// Update mouse point
	pre_mouse_x = mouse_x;
	pre_mouse_y = mouse_y;
}
/* OpenGL Initializer */
GLFWwindow* window;
bool initGL(){
	// Initialise GLFW
	if(!glfwInit()){
		cerr << "Failed to initialize GLFW." << endl;
		return false;
	}
	// Select OpenGL 3.3 Core Profile
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(WIDTH, HEIGHT, "Title", NULL, NULL);
	if(window == NULL){
		cerr << "Failed to open GLFW window. " << endl;
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	// Set vsync interval
	glfwSwapInterval(VSYNC_INTERVAL);

	// Callback
	glfwSetWindowSizeCallback(window, reshapeFunc);
	glfwSetKeyCallback(window, keyboardFunc);
	glfwSetMouseButtonCallback(window, clickFunc);
	glfwSetCursorPosCallback(window, motionFunc);


	// Initialize GLEW
	glewExperimental = true; // for core profile
	if(glewInit() != GLEW_OK){
		cerr << "Failed to initialize GLEW." << endl;
		return false;
	}

	return true;
}

/* Main */
int main(int argc, char const* argv[]){
	if(argc == 1) {
		cout << endl;
		cout << " > usage: ./render.out [mesh.obj]" << endl;
		cout << endl;
	} else {
		OBJ_FILE = argv[1];
	}

	// Load Obj file
	cout << "* Loading obj file." << endl;
	vector<vec3> triangle_buff; // |v0,v1,v2| * tri_idx
	vector<vec3> normal_buff;   // |n0,n1,n2| * tri_idx
	vector<vec2> texcoord_buf;   // |u,v| * tri_idx
	vector<int>  mat_idx_buff;  // |mat_idx| * tri_idx
	vector<vec3> material_buff; // |Kd, Ks| * mat_idx
	{
		if(!loadObjFile(OBJ_FILE, triangle_buff, normal_buff, texcoord_buf,
		                mat_idx_buff, material_buff)) return 1;
		// Clamp triangle_buff to [0,1]
		float point_scale = getClampScale(triangle_buff); // base scale
		vec3 min_point = getMinPoint(triangle_buff); // base min_point
		transformVec(triangle_buff, point_scale, min_point * -1.f);
	}

	cout << " >> " << triangle_buff.size()/3 << " triangles" << endl;

	// BVH
	cout << "* Building BVH." << endl;
	BVH bvh;
	bvh.build(triangle_buff);
	vector<int> bbox_tri_array; // triangle indices in bvh order
	vector<vec3> bbox_minmax_array;  // |min, max| * bbox_idx
	vector<int> bbox_tri_idx_array;  // |start_idx, end_idx| * bbox_idx
	vector<int> bbox_miss_idx_array; // |miss_idx| * bbox_idx
	bvh.getInfo(bbox_minmax_array, bbox_tri_array, bbox_tri_idx_array,
	            bbox_miss_idx_array);
	cout << " >> " << bbox_minmax_array.size()/2 << " bboxes" << endl;

	// Sort triangle_buff in bvh order
	vector<vec3> orl_triangle_buff = triangle_buff;
	vector<vec3> orl_normal_buff = normal_buff;
	vector<vec2> orl_texcoord_buf = texcoord_buf;
	vector<int> orl_mat_idx_buff = mat_idx_buff;
	for(int i = 0; i < bbox_tri_array.size(); i++){
		mat_idx_buff[i] = orl_mat_idx_buff[bbox_tri_array[i]];
		triangle_buff[3*i+0] = orl_triangle_buff[3*bbox_tri_array[i]+0];
		triangle_buff[3*i+1] = orl_triangle_buff[3*bbox_tri_array[i]+1];
		triangle_buff[3*i+2] = orl_triangle_buff[3*bbox_tri_array[i]+2];
		normal_buff[3*i+0] = orl_normal_buff[3*bbox_tri_array[i]+0];
		normal_buff[3*i+1] = orl_normal_buff[3*bbox_tri_array[i]+1];
		normal_buff[3*i+2] = orl_normal_buff[3*bbox_tri_array[i]+2];
		texcoord_buf[3*i+0] = orl_texcoord_buf[3*bbox_tri_array[i]+0];
		texcoord_buf[3*i+1] = orl_texcoord_buf[3*bbox_tri_array[i]+1];
		texcoord_buf[3*i+2] = orl_texcoord_buf[3*bbox_tri_array[i]+2];
	}

	// Join arrays
	vector<int> bbox_info_array;  // |start_idx, end_idx, miss_idx| * bbox_idx
	joinVectors(bbox_tri_idx_array, bbox_miss_idx_array, bbox_info_array, 2, 1);


	// Init OpenGL
	cout << "* Initializing OpenGL." << endl;
	if(!initGL()) return 1;

	// Compile shader
	cout << "* Compiling shaders." << endl;
	GLuint program_id = loadShaders(VS_FILE, FS_FILE);
	if(program_id == 0) return 1;

	cout << "* Generating gl varients." << endl;
	// Vertex Array Object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// ===== Buffers =====
	// Vertex Buffer
	vector<vec2> vertex_positions(4);
	vertex_positions[0] = vec2(-1.0f, -1.0f);
	vertex_positions[1] = vec2( 1.0f, -1.0f);
	vertex_positions[2] = vec2( 1.0f,  1.0f); 
	vertex_positions[3] = vec2(-1.0f,  1.0f);
	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	bindVertexAttribute(0, vertex_buffer, vertex_positions, GL_FLOAT, GL_STATIC_DRAW);

	// ===== Uniforms =====
	GLuint bbox_size_id = glGetUniformLocation(program_id, "bbox_size");
	GLuint camera_org_id = glGetUniformLocation(program_id, "camera_org");
	GLuint camera_dir_base_id = glGetUniformLocation(program_id, "camera_dir_base");
	GLuint camera_xvec_id = glGetUniformLocation(program_id, "camera_xvec");
	GLuint camera_yvec_id = glGetUniformLocation(program_id, "camera_yvec");
	GLuint screen_size_id = glGetUniformLocation(program_id, "screen_size");
	GLuint rand_vec2_a_id = glGetUniformLocation(program_id, "rand_vec2_a");
	GLuint rand_vec2_b_id = glGetUniformLocation(program_id, "rand_vec2_b");
	GLuint rand_vec3_id = glGetUniformLocation(program_id, "rand_vec3");
	GLuint accum_frame_id = glGetUniformLocation(program_id, "accum_frame");

	// ===== Textures =====
	// General
	TextureRect triangle_tex(1, 3*TRI_TEX_COL, triangle_buff.size()/3/TRI_TEX_COL+1, GL_RGB, GL_RGB, GL_FLOAT);//triangle
	triangle_tex.setBuffer(&triangle_buff[0]);
	TextureRect normal_tex(2, 3*TRI_TEX_COL, normal_buff.size()/3/TRI_TEX_COL+1, GL_RGB, GL_RGB, GL_FLOAT);//normal
	normal_tex.setBuffer(&normal_buff[0]);
	TextureRect texcoord_tex(3, 3*TRI_TEX_COL, texcoord_buf.size()/3/TRI_TEX_COL+1, GL_RG, GL_RG, GL_FLOAT);//texcoord
	texcoord_tex.setBuffer(&texcoord_buf[0]);
	TextureRect mat_idx_tex(4, 1*M_ID_TEX_COL, mat_idx_buff.size()/M_ID_TEX_COL + 1, GL_R32I, GL_RED_INTEGER, GL_INT);//material_idx
	mat_idx_tex.setBuffer(&mat_idx_buff[0]);
	TextureRect material_tex(5, 2*M_TEX_COL, material_buff.size()/2/M_TEX_COL + 1, GL_RGB, GL_RGB, GL_FLOAT);//material
	material_tex.setBuffer(&material_buff[0]);
	TextureRect accum_pixel_tex(6, WIDTH, HEIGHT, GL_RGB, GL_RGB, GL_FLOAT);//accum_pixel
	// BVH
	TextureRect bbox_minmax_tex(7, 2*BVH_TEX_COL, bbox_minmax_array.size()/2/BVH_TEX_COL+1, GL_RGB, GL_RGB, GL_FLOAT);//bbox_minmax
	bbox_minmax_tex.setBuffer(&bbox_minmax_array[0]);
	TextureRect bbox_info_tex(8, 3*BVH_TEX_COL, bbox_info_array.size()/3/BVH_TEX_COL+1, GL_R32I, GL_RED_INTEGER, GL_INT);//bbox triangle idx
	bbox_info_tex.setBuffer(&bbox_info_array[0]);

	// ===== Main loop =====
	cout << "* Start rendering." << endl;
	accum_frame = 0;
	FpsCounter fps;
	while(glfwWindowShouldClose(window) == GL_FALSE) {
		// Accumulator
		if(accum_frame == 0){
		} else {
			accum_pixel_tex.copyPixels(WIDTH, HEIGHT);
		}

		// FPS
		fps.update();

		// Camera
		vec3 dir_base, x_vec, y_vec;
		vec3 camera_org = camera.getOrg();
		camera.getScreenInf(WIDTH, HEIGHT, dir_base, x_vec, y_vec);

		// ===== Buffers =====
		glEnableVertexAttribArray(0);
		// ===== Uniforms =====
		glUniform1i(bbox_size_id, bbox_minmax_array.size()/2);
		glUniform3f(camera_org_id, camera_org.x, camera_org.y, camera_org.z);
		glUniform3f(camera_dir_base_id, dir_base.x, dir_base.y, dir_base.z);
		glUniform3f(camera_xvec_id, x_vec.x, x_vec.y, x_vec.z);
		glUniform3f(camera_yvec_id, y_vec.x, y_vec.y, y_vec.z);
		glUniform2f(screen_size_id, WIDTH, HEIGHT);
		glUniform2f(rand_vec2_a_id, random()/float(RAND_MAX), random()/float(RAND_MAX));
		glUniform2f(rand_vec2_b_id, random()/float(RAND_MAX), random()/float(RAND_MAX));
		glUniform3f(rand_vec3_id, rand()/float(RAND_MAX), rand()/float(RAND_MAX), rand()/float(RAND_MAX));
		glUniform1i(accum_frame_id, accum_frame);
		// ===== Textures =====
		// General
		triangle_tex.active();
		triangle_tex.bindUniform(program_id, "triangle_tex");
		normal_tex.active();
		normal_tex.bindUniform(program_id, "normal_tex");
		texcoord_tex.active();
		texcoord_tex.bindUniform(program_id, "texcoord_tex");
		mat_idx_tex.active();
		mat_idx_tex.bindUniform(program_id, "mat_idx_tex");
		material_tex.active();
		material_tex.bindUniform(program_id, "material_tex");
		accum_pixel_tex.active();
		accum_pixel_tex.bindUniform(program_id, "accum_pixel_tex");
		// BVH
		bbox_minmax_tex.active();
		bbox_minmax_tex.bindUniform(program_id, "bbox_minmax_tex");
		bbox_info_tex.active();
		bbox_info_tex.bindUniform(program_id, "bbox_info_tex");

		accum_frame++;// next frame

		// ====== Draw =====
		// Clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Use shader
		glUseProgram(program_id);
		// Draw
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		// Swap screen buffers
		glfwSwapBuffers(window);
		// Poll callbacks
		glfwPollEvents();
	}

	// ===== Termination Process =====
	// Cleanup shader, VAO and buffers.
	glDeleteProgram(program_id);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vertex_buffer);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

