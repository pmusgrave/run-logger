#ifndef RUNLOGGER_HPP
#define RUNLOGGER_HPP

#include "run.hpp"
#include <vector>

#define LED 13
#define START_PAUSE_BUTTON 14 // subject to change
#define STOP_PAUSE_BUTTON 15 // subject to change
#define RESET_PAUSE_BUTTON 16 // subject to change

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
	void handle_new_gps_coord(double lat, double lon);
	
private:
	volatile uint8_t state;
	void push_data_to_cloud(void);
};

#endif
