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
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	if (duration.tv_sec == 0.0 && duration.tv_usec == 0.0) {
		run_start_time = current_time;
		most_recent_pause_time = current_time;
	}
	most_recent_start_time = current_time;

	struct timeval timediff;
	timersub(&current_time, &most_recent_pause_time, &timediff);
	timeradd(&total_paused_duration, &timediff, &total_paused_duration);
	// std::cout << "Starting run. Current time: "
	// 	<< asctime(gmtime(&current_time))
	// 	<< std::endl;
}

void Run::pause(void) {
	paused = true;
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	most_recent_pause_time = current_time;
	struct timeval timediff;
	timersub(&current_time, &most_recent_start_time, &timediff);
	timeradd(&duration, &timediff, &duration);
}

void Run::reset(void) {
	in_progress = false;
	paused = true;
	timerclear(&run_start_time);
	timerclear(&duration);
	timerclear(&total_paused_duration);
	timerclear(&most_recent_start_time);
	timerclear(&most_recent_pause_time);
	distance = 0.0;
}

void Run::stop(void) {
	if(!paused){
		pause();
	}
	in_progress = false;
}

bool Run::is_in_progress(void) {
	return in_progress;
}

bool Run::is_paused(void) {
	return paused;
}

struct timeval Run::get_duration(void) const {
	if ( in_progress && (!paused) )  {
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		struct timeval current_duration;
		timersub(&current_time, &run_start_time, &current_duration);
		timersub(&current_duration, &total_paused_duration, &current_duration);
	    return current_duration;	
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
