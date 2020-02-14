#include "unity.h"
#include "run.hpp"

TEST_CASE("Run class initialization", "[run]") {
	Run test_run;
	TEST_ASSERT_EQUAL(false, test_run.is_in_progress());
	TEST_ASSERT_EQUAL(0, test_run.get_duration());
	TEST_ASSERT_EQUAL(0, test_run.get_distance());
}
