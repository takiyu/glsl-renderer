#include "fps_counter.h"

using namespace std;

/* FPS Counter*/
FpsCounter::FpsCounter(){
	init();
}
void FpsCounter::init(){
	last_time = glfwGetTime();
	frame_count = 0;
}
void FpsCounter::update(){
	double current_time = glfwGetTime();
	frame_count++;
	if ( current_time - last_time >= 1.0 ){
		cout << frame_count << "fps" << endl;
		frame_count = 0;
		last_time += 1.0;
	}
}
