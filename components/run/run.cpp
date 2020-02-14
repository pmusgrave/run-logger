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

bool Run::is_in_progress(void) {
	return this->in_progress;
}

double Run::get_duration(void) {
	if (in_progress) {
		time_t current_time = time(NULL);
		std::cout << "Current time: " 
			<< asctime(gmtime(&current_time)) 
			<< std::endl;
		time_t current_duration = current_time - this->run_start_time;
		std::cout << "Getting duration. Current difference: "
	            << current_duration
	            << std::endl;
	    return current_duration;	
	}
	else {
		return this->duration;
	}
}

double Run::get_distance(void) {
	return this->distance;
}
