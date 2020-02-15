#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include <vector>

class RunLogger {
public:
	RunLogger();
	~RunLogger();
	Run current_run;
  
private:
	std::vector<Run> log;
};

#endif
