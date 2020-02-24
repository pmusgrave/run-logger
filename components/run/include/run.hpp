#ifndef RUN_HPP
#define RUN_HPP

// All distance units in meters unless otherwise specified

#include <ctime>
#include <vector>

typedef struct coord_t {
	double lat;
	double lon;
} coord_t;

class Run {
public:
	Run();
	~Run();
	void start(void);
	void pause(void);
	void stop(void);
	void reset(void);
	bool is_in_progress(void);
	bool is_paused(void);
	struct tm get_start_date(void) const;
	struct timeval get_duration(void) const;
	double get_distance(void) const;
	void add_gps_point_to_distance(double lat, double lon);
	Run clone(void) const;

private:
	bool in_progress;
	bool paused;
	struct timeval run_start_time;
	struct timeval most_recent_start_time;
	struct timeval most_recent_pause_time;
	struct timeval duration;
	struct timeval total_paused_duration;
	double distance;
	std::vector<coord_t> coords;

	inline double degrees_to_radians(double val);
};

#endif
