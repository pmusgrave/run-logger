#include <ctime>

class Run {
public:
	Run();
	~Run();
	void get_run_status(void);

private:
	bool in_progress;
	time_t run_start_time;
	double duration;
	uint32_t distance;
};