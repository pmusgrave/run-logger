#include <iostream>
#include "esp_sleep.h"
#include "unity.h"
#include "runlogger.hpp"

TEST_CASE("Runlogger class initialization", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(false, test_logger.current_run.is_in_progress());
	TEST_ASSERT_EQUAL(true, test_logger.current_run.is_paused());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_distance());
}

TEST_CASE("GPIO initialization and handling", "[runlogger]") {
	RunLogger test_logger;
}

TEST_CASE("Runlogger stop", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = &test_logger.current_run;

	test_run->start();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();

	test_logger.complete_run();

	Run& last_run = test_logger.log.back();
	TEST_ASSERT_EQUAL(last_run.is_in_progress(), test_run->is_in_progress());
	TEST_ASSERT_EQUAL(last_run.is_paused(), test_run->is_paused());
	TEST_ASSERT_EQUAL(last_run.get_duration(), test_run->get_duration());
	TEST_ASSERT_EQUAL(last_run.get_distance(), test_run->get_distance());
	TEST_ASSERT_EQUAL(1, test_logger.log.size());
}

TEST_CASE("Runlogger push to cloud", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = &test_logger.current_run;

	test_run->start();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();

	test_logger.complete_run();

	Run& last_run = test_logger.log.back();
}
