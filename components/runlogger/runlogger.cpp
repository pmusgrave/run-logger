#include "runlogger.hpp"
#include "run.hpp"
#include <iostream>

RunLogger::RunLogger(){
  // std::cout << "Initializing: Reserving space for 10 runs" << std::endl;
  log.reserve(10);
}
RunLogger::~RunLogger(){
	// std::cout << "Destructing runlogger" << std::endl;
}

void RunLogger::complete_run(void) {
	current_run.stop();
	log.push_back(current_run.clone());
}