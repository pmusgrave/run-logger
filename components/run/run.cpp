#include <iostream>
#include <ctime>
#include "run.hpp"

Run::Run() {
	in_progress = false;
	paused = true;
	run_start_time = 0;
	duration = 0.0;
	distance = 0.0;
	total_paused_duration = 0.0;
	most_recent_start_time = 0.0;
	most_recent_pause_time = 0.0;
	time(NULL);
}
Run::~Run() {}

void Run::start(void) {
	in_progress = true;
	paused = false;
	time_t current_time = time(NULL);

	if (duration == 0.0) {
		run_start_time = current_time;
		most_recent_pause_time = current_time;
	}
	most_recent_start_time = current_time;

	total_paused_duration += (current_time - most_recent_pause_time);
	std::cout << "Starting run. Current time: "
		<< asctime(gmtime(&current_time))
		<< "Pause time: "
		<< most_recent_pause_time
		<< std::endl;
}

void Run::pause(void) {
	paused = true;
	time_t current_time = time(NULL);
	most_recent_pause_time = current_time;
	duration += current_time - most_recent_start_time;
}

bool Run::is_in_progress(void) {
	return this->in_progress;
}

bool Run::is_paused(void) {
	return this->paused;
}

double Run::get_duration(void) {
	if ( in_progress && (!paused) )  {
		time_t current_time = time(NULL);
		time_t current_duration = current_time - this->run_start_time;
	    return current_duration - total_paused_duration;	
	}
	else {
		return this->duration;
	}
}

double Run::get_distance(void) {
	return this->distance;
}
