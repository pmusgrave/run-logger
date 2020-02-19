#include "runlogger.hpp"
#include "run.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <iostream>


RunLogger::RunLogger(){
	init_io();
	// std::cout << "Initializing: Reserving space for 10 runs" << std::endl;
	log.reserve(10);
	state = RESET;
}
RunLogger::~RunLogger(){
	// std::cout << "Destructing runlogger" << std::endl;
}

void RunLogger::init_io(void) {
	std::cout << "Initializing I/O" << std::endl;
    // configure start/pause button
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL<<START_PAUSE_BUTTON);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (gpio_pullup_t)1;
    gpio_config(&io_conf);

    // indicator LED as output
    gpio_pad_select_gpio((gpio_num_t)13);
    gpio_set_direction((gpio_num_t)13, GPIO_MODE_OUTPUT);
}

uint8_t RunLogger::get_state(void) {
	return this->state;
}

// start or pause the run, depending on state
void RunLogger::handle_start_pause_button(void){
	switch(state) {
	case RESET:
		current_run.start();
		state = RUN_IN_PROGRESS;	
		break;
	case RUN_IN_PROGRESS:
		current_run.pause();
		state = PAUSED;
		break;
	case PAUSED:
		current_run.start();
		state = RUN_IN_PROGRESS;
	case STOPPED:
		// what should happen here? Reset then start?
		this->handle_reset_button();
		current_run.start();
		state = RUN_IN_PROGRESS;
		break;
	default:
		break;
	}
}

// stop timer, log current run's data, don't clear current run
void RunLogger::handle_stop_button(void){
	switch(state) {
	case RESET:
		break;
	case RUN_IN_PROGRESS:
		current_run.stop();
		log.push_back(current_run.clone());
		break;
	case PAUSED:
		current_run.stop();
		log.push_back(current_run.clone());
		break;
	case STOPPED:
		break;
	default:
		current_run.stop();
		log.push_back(current_run.clone());
		break;
	}
	
	state = STOPPED;
}

// stop timer, do not log current run's data, clear current run
void RunLogger::handle_reset_button(void){
	current_run.reset();
	state = RESET;
}

void RunLogger::push_data_to_cloud(void) {
	
}
