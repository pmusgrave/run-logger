#include "runlogger.hpp"
#include "run.hpp"
#include <iostream>

RunLogger::RunLogger(){
  std::cout << "Initializing: Reserving space for 10 runs" << std::endl;
  log.reserve(10);
}
RunLogger::~RunLogger(){}

Run RunLogger::get_current_run () {
  return this->current_run;
}

void RunLogger::start_run() {
  current_run.start();
}
