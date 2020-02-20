#ifndef RUN_HPP
#define RUN_HPP

#include <ctime>

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
	struct timeval get_duration(void) const;
	double get_distance(void) const;
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
};

#endif
