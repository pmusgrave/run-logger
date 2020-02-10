#include <ctime>
#include "run.hpp"

Run::Run() {
	in_progress = false;
	run_start_time = time(0);
	duration = 0.0;
	distance = 0.0;
}
Run::~Run() {}

bool Run::get_status() {
	return this->in_progress;
}

double Run::get_duration(void) {
	return this->duration;
}

double Run::get_distance(void) {
	return this->distance;
}