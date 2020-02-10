#include "unity.h"
#include "run.hpp"

TEST_CASE("Run class initialization", "[init]") {
	Run test_run;
	TEST_ASSERT_EQUAL(false, test_run.get_run_status());
	TEST_ASSERT_EQUAL(0, test_run.get_run_duration());
	TEST_ASSERT_EQUAL(0, test_run.get_run_distance());
}
