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
	double get_duration(void) const;
	double get_distance(void) const;
	Run clone(void) const;

private:
	bool in_progress;
	bool paused;
	time_t run_start_time;
	time_t most_recent_start_time;
	time_t most_recent_pause_time;
	double duration;
	double distance;
	double total_paused_duration;
};

#endif
