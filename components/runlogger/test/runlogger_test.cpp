#include "unity.h"
#include "runlogger.hpp"

TEST_CASE("Runlogger class initialization", "[init]") {
  RunLogger test_logger;
  TEST_ASSERT_EQUAL(false, test_logger.get_current_run().get_status());
	TEST_ASSERT_EQUAL(0, test_logger.get_current_run().get_duration());
  TEST_ASSERT_EQUAL(0, test_logger.get_current_run().get_distance());
}

TEST_CASE("Runlogger start run", "[start_run]") {
  RunLogger test_logger;
  test_logger.start_run();
  TEST_ASSERT_EQUAL(true, test_logger.get_current_run().get_status());
	TEST_ASSERT_NOT_EQUAL(0, test_logger.get_current_run().get_duration());
  TEST_ASSERT_NOT_EQUAL(0, test_logger.get_current_run().get_distance());
}
