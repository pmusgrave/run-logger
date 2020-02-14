#ifndef RUN_HPP
#define RUN_HPP

#include <ctime>

class Run {
public:
	Run();
	~Run();
	bool get_status(void);
	double get_duration(void);
	double get_distance(void);

private:
	bool in_progress;
	time_t run_start_time;
	double duration;
	double distance;
};

#endif