#include <iostream>
#include "esp_sleep.h"
#include "unity.h"
#include "runlogger.hpp"

TEST_CASE("Runlogger class initialization", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(false, test_logger.current_run.is_in_progress());
	TEST_ASSERT_EQUAL(true, test_logger.current_run.is_paused());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration().tv_sec);
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_distance());
}

TEST_CASE("GPIO initialization and handling", "[runlogger]") {
	RunLogger test_logger;
}

TEST_CASE("Runlogger start/pause button", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(PAUSED, test_logger.get_state());
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());

	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
}

TEST_CASE("Runlogger stop button", "[runlogger]") {
	RunLogger test_logger;
	Run* test_run = &test_logger.current_run;

	// RUN_IN_PROGRESS -> stop_button()
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	test_logger.handle_start_pause_button();	
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(1, test_logger.current_run.get_duration().tv_sec);
	Run& last_run = test_logger.log.back();
	TEST_ASSERT_EQUAL(last_run.is_in_progress(), test_run->is_in_progress());
	TEST_ASSERT_EQUAL(last_run.is_paused(), test_run->is_paused());
	TEST_ASSERT_EQUAL(last_run.get_duration().tv_sec, test_run->get_duration().tv_sec);
	TEST_ASSERT_EQUAL(last_run.get_distance(), test_run->get_distance());
	TEST_ASSERT_EQUAL(1, test_logger.log.size());

	// start a new run without pressing reset button
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	esp_sleep_enable_timer_wakeup(2000000LL);
	esp_light_sleep_start();
	test_logger.handle_start_pause_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(PAUSED, test_logger.get_state());
	TEST_ASSERT_EQUAL(2, test_logger.current_run.get_duration().tv_sec);
	test_logger.handle_stop_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(2, test_logger.current_run.get_duration().tv_sec);
	Run& last_run2 = test_logger.log.back();
	TEST_ASSERT_EQUAL(last_run2.is_in_progress(), test_run->is_in_progress());
	TEST_ASSERT_EQUAL(last_run2.is_paused(), test_run->is_paused());
	TEST_ASSERT_EQUAL(last_run2.get_duration().tv_sec, test_run->get_duration().tv_sec);
	TEST_ASSERT_EQUAL(last_run2.get_distance(), test_run->get_distance());
	TEST_ASSERT_EQUAL(2, test_logger.log.size());

	// press reset button and then start a new run
	test_logger.handle_reset_button();
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	esp_sleep_enable_timer_wakeup(3000000LL);
	esp_light_sleep_start();
	test_logger.handle_start_pause_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(PAUSED, test_logger.get_state());
	TEST_ASSERT_EQUAL(3, test_logger.current_run.get_duration().tv_sec);
	test_logger.handle_stop_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(3, test_logger.current_run.get_duration().tv_sec);
	Run& last_run3 = test_logger.log.back();
	TEST_ASSERT_EQUAL(last_run3.is_in_progress(), test_run->is_in_progress());
	TEST_ASSERT_EQUAL(last_run3.is_paused(), test_run->is_paused());
	TEST_ASSERT_EQUAL(last_run3.get_duration().tv_sec, test_run->get_duration().tv_sec);
	TEST_ASSERT_EQUAL(last_run3.get_distance(), test_run->get_distance());
	TEST_ASSERT_EQUAL(3, test_logger.log.size());

	// PAUSED -> stop_button()
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	test_logger.handle_start_pause_button();	
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	esp_sleep_enable_timer_wakeup(4000000LL);
	esp_light_sleep_start();
	test_logger.handle_start_pause_button();	
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(4, test_logger.current_run.get_duration().tv_sec);
	Run& last_run4 = test_logger.log.back();
	TEST_ASSERT_EQUAL(last_run4.is_in_progress(), test_run->is_in_progress());
	TEST_ASSERT_EQUAL(last_run4.is_paused(), test_run->is_paused());
	TEST_ASSERT_EQUAL(last_run4.get_duration().tv_sec, test_run->get_duration().tv_sec);
	TEST_ASSERT_EQUAL(last_run4.get_distance(), test_run->get_distance());
	TEST_ASSERT_EQUAL(4, test_logger.log.size());

	// RESET -> stop_button()
	// should not store another item in log
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(4, test_logger.log.size());

	// pressing stop button more than once
	test_logger.handle_reset_button();
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(5, test_logger.log.size());
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(5, test_logger.log.size());
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	TEST_ASSERT_EQUAL(5, test_logger.log.size());
}

TEST_CASE("Runlogger reset button", "[runlogger]") {
	RunLogger test_logger;
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	test_logger.handle_start_pause_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	TEST_ASSERT_EQUAL(1, test_logger.current_run.get_duration().tv_sec);
	// RUN_IN_PROGRESS -> reset_button()
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration().tv_sec);
	test_logger.handle_start_pause_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	TEST_ASSERT_EQUAL(1, test_logger.current_run.get_duration().tv_sec);
	test_logger.handle_start_pause_button();
	TEST_ASSERT_EQUAL(PAUSED, test_logger.get_state());
	// PAUSED -> reset_button()
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration().tv_sec);
	test_logger.handle_start_pause_button();
	esp_sleep_enable_timer_wakeup(1000000LL);
	esp_light_sleep_start();
	TEST_ASSERT_EQUAL(RUN_IN_PROGRESS, test_logger.get_state());
	test_logger.handle_stop_button();
	TEST_ASSERT_EQUAL(1, test_logger.current_run.get_duration().tv_sec);
	TEST_ASSERT_EQUAL(STOPPED, test_logger.get_state());
	// STOPPED -> reset_button()
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration().tv_sec);
	// RESET -> reset_button()
	test_logger.handle_reset_button();
	TEST_ASSERT_EQUAL(RESET, test_logger.get_state());
	TEST_ASSERT_EQUAL(0, test_logger.current_run.get_duration().tv_sec);
}

TEST_CASE("Runlogger push to cloud", "[runlogger]") {

}
