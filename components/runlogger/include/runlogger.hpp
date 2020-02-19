#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include <vector>

#define START_PAUSE_BUTTON 14

enum app_state {RESET, RUN_IN_PROGRESS, PAUSED, STOPPED};

class RunLogger {
public:
	RunLogger();
	~RunLogger();
	Run current_run;
	std::vector<Run> log;
	void init_io(void);
	uint8_t get_state(void);
	void handle_start_pause_button(void);
	void handle_stop_button(void);
	void handle_reset_button(void);
	
private:
	uint8_t state;
	void push_data_to_cloud(void);
};

#endif
