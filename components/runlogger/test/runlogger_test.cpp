#include "unity.h"
#include "runlogger.hpp"
#include <iostream>
#include "esp_sleep.h"

void setUp(){

}

void tearDown() {

}

TEST_CASE("Runlogger class initialization", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(false, test_logger.get_current_run()->is_in_progress());
	TEST_ASSERT_EQUAL(0, test_logger.get_current_run()->get_duration());
	TEST_ASSERT_EQUAL(0, test_logger.get_current_run()->get_distance());
}

TEST_CASE("Runlogger start run", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = test_logger.get_current_run();
	test_logger.start_run();
	std::cout << "Entering deep sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run->is_in_progress());
	TEST_ASSERT_EQUAL(0, test_run->get_distance());
	TEST_ASSERT_NOT_EQUAL(0, test_run->get_duration());
}
