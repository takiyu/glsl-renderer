#ifndef FPS_COUNTER_H_150331
#define FPS_COUNTER_H_150331

#include <iostream>
#include <GLFW/glfw3.h>

class  FpsCounter {
public:
	FpsCounter();
	void init();
	void update();

private:
	double last_time;
	int frame_count;
};
#endif
