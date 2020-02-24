#include <iostream>
#include <ctime>
#include <cmath>
#include "run.hpp"

Run::Run() {
	coords.reserve(100);
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

struct tm Run::get_start_date(void) const {
	return *localtime(&run_start_time.tv_sec);
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

void Run::add_gps_point_to_distance(double lat, double lon) {
	if(!in_progress || paused) {
		return;
	}
	if (coords.size() == 0) {
		coord_t new_coord = {lat,lon};
		coords.push_back(new_coord);
		return;
	}

	coord_t last_known_coord = coords.back();
	coord_t new_coord = {lat,lon};
	coords.push_back(new_coord);

	const double earth_radius = 6371000;
	double delta_lat = degrees_to_radians(lat - last_known_coord.lat);
	double delta_lon = degrees_to_radians(lon - last_known_coord.lon);
	double lat1 = degrees_to_radians(lat);
	double lat2 = degrees_to_radians(last_known_coord.lat);

	double a = sin(delta_lat / 2) * sin(delta_lat / 2)
		+ sin(delta_lon/2) * sin(delta_lon/2) * cos(lat1) * cos(lat2);
	double c = 2 * atan2(sqrt(a), sqrt(1-a)); 
	double distance_delta = earth_radius * c;
	distance += abs(distance_delta);
}

inline double Run::degrees_to_radians(double val) {
	return val * M_PI / 180;
}
