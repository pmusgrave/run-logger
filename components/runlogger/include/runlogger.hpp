#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
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
