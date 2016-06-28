#ifndef CAMERA_H_150313
#define CAMERA_H_150313

#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const static float HALF_PIE = 3.1415f * 0.5f;
const static float SCREEN_DIST = 1.0f;

class Camera {
public:
	Camera();
	Camera(const glm::vec3& from);
	glm::vec3 getOrg(){ return this->from; }
	void move(float dx, float dy, float dz);
	void rotateOrbit(float dtheta, float dphi);
	void getScreenInf(const int width, const int height, glm::vec3& dir_base, glm::vec3& x_vec, glm::vec3& y_vec);
private:
	glm::vec3 from, to, up;
	float fov;//field of view
};
#endif
