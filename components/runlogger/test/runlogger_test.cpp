#include "unity.h"
#include "runlogger.hpp"
#include <iostream>
#include "esp_sleep.h"

TEST_CASE("Runlogger class initialization", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(false, test_logger.current_run.is_in_progress());
	TEST_ASSERT_EQUAL(true, test_logger.current_run.is_paused());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_distance());
}

TEST_CASE("Runlogger start run", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = &test_logger.current_run;
	
	test_run->start();
	
	std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run->is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run->is_paused());
	TEST_ASSERT_EQUAL(1, test_run->get_duration());
}

TEST_CASE("Runlogger pause", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = &test_logger.current_run;

	test_run->start();

	std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run->is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run->is_paused());
	TEST_ASSERT_EQUAL(1, test_run->get_duration());
	
	test_run->pause();
	double duration = test_run->get_duration();

	std::cout << "Entering sleep for 1 seconds" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
    TEST_ASSERT_EQUAL(true, test_run->is_in_progress()); // ? Should it be in progress when paused? I think so
	TEST_ASSERT_EQUAL(true, test_run->is_paused());
	TEST_ASSERT_EQUAL(1, duration);
	TEST_ASSERT_EQUAL(duration, test_run->get_duration()); // Duration stored before 1s sleep should be the same

	// Start and pause a second time
	test_run->start();

	std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run->is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run->is_paused());
	TEST_ASSERT_EQUAL(2, test_run->get_duration());
	
	test_run->pause();
	duration = test_run->get_duration();

	std::cout << "Entering sleep for 1 seconds" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
    TEST_ASSERT_EQUAL(true, test_run->is_in_progress()); // ? Should it be in progress when paused? I think so
	TEST_ASSERT_EQUAL(true, test_run->is_paused());
	TEST_ASSERT_EQUAL(2, duration);
	TEST_ASSERT_EQUAL(duration, test_run->get_duration()); // Duration stored before 1s sleep should be the same
}
