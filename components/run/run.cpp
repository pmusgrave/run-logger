#include <iostream>
#include <ctime>
#include "run.hpp"

Run::Run() {
	reset();
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
	// std::cout << "Starting run. Current time: "
	// 	<< asctime(gmtime(&current_time))
	// 	<< std::endl;
}

void Run::pause(void) {
	paused = true;
	time_t current_time = time(NULL);
	most_recent_pause_time = current_time;
	duration += current_time - most_recent_start_time;
}

void Run::reset(void) {
	in_progress = false;
	paused = true;
	run_start_time = 0;
	duration = 0.0;
	distance = 0.0;
	total_paused_duration = 0.0;
	most_recent_start_time = 0.0;
	most_recent_pause_time = 0.0;
}

void Run::stop(void) {
	in_progress = false;
	pause();
}

bool Run::is_in_progress(void) {
	return in_progress;
}

bool Run::is_paused(void) {
	return paused;
}

double Run::get_duration(void) const {
	if ( in_progress && (!paused) )  {
		time_t current_time = time(NULL);
		time_t current_duration = current_time - run_start_time;
	    return current_duration - total_paused_duration;	
	}
	else {
		return duration;
	}
}

double Run::get_distance(void) const {
	return distance;
}

Run Run::clone() const {
	Run clone;
	clone.in_progress = this->in_progress;
	clone.paused = this->paused;
	clone.run_start_time = this->run_start_time;
	clone.most_recent_start_time = this->most_recent_start_time;
	clone.most_recent_pause_time = this->most_recent_pause_time;
	clone.duration = this->duration;
	clone.distance = this->distance;
	clone.total_paused_duration = this->total_paused_duration;
	return clone;
}
