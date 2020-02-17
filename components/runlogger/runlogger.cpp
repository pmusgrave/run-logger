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
}
RunLogger::~RunLogger(){
	// std::cout << "Destructing runlogger" << std::endl;
}

void RunLogger::init_io(void) {

}

void RunLogger::complete_run(void) {
	current_run.stop();
	log.push_back(current_run.clone());
}

void RunLogger::push_data_to_cloud(void) {
	
}
