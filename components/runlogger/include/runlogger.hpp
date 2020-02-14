#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include <vector>

// This is the class that handles main UI
class RunLogger {
public:
	RunLogger();
	~RunLogger();
	void start_run(void);
	void pause_run(void);
	void stop_run(void);
	void reset(void);
	Run* get_current_run();
  
private:
	Run current_run;
	std::vector<Run> log;
};

#endif
