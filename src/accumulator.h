#ifndef UNIT_PIXEL_H_150226
#define UNIT_PIXEL_H_150226

#include <vector>
#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Accumulator {
public:
	const static int MAX_UNIT_NUM = 10;

	Accumulator();
	void setScreenSize(int width, int height);
	void clear();
	void update(int x, int y, const glm::vec3& c, const float weight);
	glm::vec3 get(int x, int y);

private:
	int width, height;
	std::vector<std::vector<glm::vec3> > data;
	std::vector<std::vector<float> > weights;
};

#endif
