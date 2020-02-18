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
	void init_io(void);
	void complete_run(void);
	void push_data_to_cloud(void);
};

#endif
