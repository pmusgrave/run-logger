#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include <vector>

class RunLogger {
public:
	RunLogger();
	~RunLogger();
	Run current_run;
	std::vector<Run> log;
	void complete_run(void);
};

#endif
