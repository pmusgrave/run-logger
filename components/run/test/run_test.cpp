#include <iostream>
#include <ctime>
#include "esp_sleep.h"
#include "unity.h"
#include "run.hpp"

TEST_CASE("Run class initialization", "[run]") {
	Run test_run;
	TEST_ASSERT_EQUAL(false, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(0, test_run.get_duration().tv_sec);
	TEST_ASSERT_EQUAL(0, test_run.get_distance());
}

TEST_CASE("Start run", "[run]") {
	Run test_run;
	test_run.start();
	
	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run.is_paused());
	TEST_ASSERT_NOT_EQUAL(0, test_run.get_duration().tv_usec);
}

TEST_CASE("Pause run", "[run]") {
	Run test_run;
	test_run.start();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run.is_paused());
	TEST_ASSERT_EQUAL(1, test_run.get_duration().tv_sec);
	
	test_run.pause();
	struct timeval duration = test_run.get_duration();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
    TEST_ASSERT_EQUAL(true, test_run.is_in_progress()); // ? Should it be in progress when paused? I think so
	TEST_ASSERT_EQUAL(true, test_run.is_paused());
	TEST_ASSERT_EQUAL(1, duration.tv_sec);
	TEST_ASSERT_EQUAL(duration.tv_sec, test_run.get_duration().tv_sec); // Duration stored before 1s sleep should be the same

	// Start and pause a second time
	test_run.start();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(true, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(false, test_run.is_paused());
	TEST_ASSERT_EQUAL(2, test_run.get_duration().tv_sec);
	
	test_run.pause();
	duration = test_run.get_duration();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
    esp_light_sleep_start();
    TEST_ASSERT_EQUAL(true, test_run.is_in_progress()); // ? Should it be in progress when paused? I think so
	TEST_ASSERT_EQUAL(true, test_run.is_paused());
	TEST_ASSERT_EQUAL(2, duration.tv_sec);
	TEST_ASSERT_EQUAL(duration.tv_sec, test_run.get_duration().tv_sec); // Duration stored before 1s sleep should be the same
}

TEST_CASE("Reset run", "[run]") {
	Run test_run;
	// do a bunch of stuff first
	test_run.start();
	test_run.pause();
	test_run.start();
	test_run.pause();
	test_run.start();
	test_run.pause();
	test_run.start();
	test_run.pause();
	test_run.start();
	test_run.pause();
	test_run.start();

	// then reset
	test_run.reset();
	TEST_ASSERT_EQUAL(false, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(true, test_run.is_paused());
	TEST_ASSERT_EQUAL(0, test_run.get_duration().tv_sec);
	TEST_ASSERT_EQUAL(0, test_run.get_distance());
}

TEST_CASE("Stop run", "[run]") {
	Run test_run;
	test_run.start();

	// std::cout << "Entering sleep for 1 second" << std::endl;
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();

	test_run.stop();

	TEST_ASSERT_EQUAL(false, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(true, test_run.is_paused());
	TEST_ASSERT_EQUAL(1, test_run.get_duration().tv_sec);
}
