#include "camera.h"

using namespace std;
using namespace glm;

Camera::Camera() : from(0.5,0.40,2.0), to(0.5,0.35,0.5), up(0,1,0), fov(45) {
}
Camera::Camera(const vec3& from) : from(from), to(0,0,0), up(0,1,0), fov(45) {
}
void Camera::move(float dx, float dy, float dz){
	vec3 dir_l = (this->to - this->from);
	vec3 right_l = cross(dir_l, this->up);
	vec3 up_l = cross(right_l, dir_l);

	dir_l   *= dx / length(dir_l);
	right_l *= dy / length(right_l);
	up_l    *= dz / length(up_l);

	this->from += dir_l + right_l + up_l;
	this->to += right_l + up_l;
}
void Camera::rotateOrbit(float dtheta, float dphi){
	vec3 dir = this->to - this->from;
	float dir_norm = length(dir);
	dir *= 1.0/dir_norm;//normalize

	float theta = atan2f(dir.x, dir.z);
	float phi   = asinf(dir.y);
	theta += dtheta;
	phi   += dphi;
	// Check phi range
	if(HALF_PIE < phi) phi = HALF_PIE;
	else if(phi < -HALF_PIE) phi = -HALF_PIE;

	dir.x = cosf(phi)*sinf(theta);
	dir.y = sinf(phi);
	dir.z = cosf(phi)*cosf(theta);

	dir *= dir_norm;//reverce normalize
	this->from = this->to - dir;
}
void Camera::getScreenInf(const int width, const int height, vec3& dir_base, vec3& x_vec, vec3& y_vec){
	float screen_half_width = abs(SCREEN_DIST * tanf(0.5f*this->fov * M_PI/180));
	float scale = (screen_half_width * 2.0f) / width;

	vec3 dir = this->to - this->from;

	x_vec = normalize(cross(dir, up));
	x_vec *= scale;//normalize

	y_vec = normalize(cross(dir,x_vec));
	y_vec *= scale;//normalize

	dir_base = dir / length(dir);
	dir_base += y_vec * (height*0.5f);
	dir_base -= x_vec * (width *0.5f);
}
