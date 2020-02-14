#include <iostream>
#include <ctime>
#include "run.hpp"

Run::Run() {
	in_progress = false;
	run_start_time = 0;
	duration = 0.0;
	distance = 0.0;
}
Run::~Run() {}

void Run::start(void) {
  this->in_progress = true;
  this->run_start_time = time(NULL);
  std::cout << "Starting run. Current time: "
            << asctime(gmtime(&this->run_start_time))
            << std::endl;
}

bool Run::get_status(void) {
	return this->in_progress;
}

double Run::get_duration(void) {
	return this->duration;
}

double Run::get_distance(void) {
	return this->distance;
}
