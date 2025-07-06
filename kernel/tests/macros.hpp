/**
 * @file tests/macros.hpp
 * @brief Test assertion macros for kernel unit testing
 * 
 * This file provides assertion and expectation macros for writing unit tests.
 * ASSERT macros will cause the test to return immediately on failure, while
 * EXPECT macros will log the failure but continue test execution.
 * 
 * @date 2024
 */

#pragma once

#include <cstddef>
#include "graphics/log.hpp"

/**
 * @brief Assert that two values are equal
 * @param x First value to compare
 * @param y Second value to compare
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_TEST("ASSERT_EQ failed: %d != %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that two values are not equal
 * @param x First value to compare
 * @param y Second value to compare
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_NE(x, y)                                                             \
	do {                                                                            \
		if ((x) == (y)) {                                                           \
			LOG_TEST("ASSERT_NE failed: %d == %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that x is greater than y
 * @param x Value that should be greater
 * @param y Value that should be less
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_GT(x, y)                                                             \
	do {                                                                            \
		if ((x) <= (y)) {                                                           \
			LOG_TEST("ASSERT_GT failed: %d <= %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that x is less than y
 * @param x Value that should be less
 * @param y Value that should be greater
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_LT(x, y)                                                             \
	do {                                                                            \
		if ((x) >= (y)) {                                                           \
			LOG_TEST("ASSERT_LT failed: %d >= %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that a condition is true
 * @param x Condition to test
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_TRUE(x)                                                              \
	do {                                                                            \
		if (!(x)) {                                                                 \
			LOG_TEST("ASSERT_TRUE failed: %d", x);                                  \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that a condition is false
 * @param x Condition to test
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_FALSE(x)                                                             \
	do {                                                                            \
		if (x) {                                                                    \
			LOG_TEST("ASSERT_FALSE failed: %d", x);                                 \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that a pointer is NULL
 * @param x Pointer to test
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_NULL(x)                                                              \
	do {                                                                            \
		if ((x) != nullptr) {                                                          \
			LOG_TEST("ASSERT_NULL failed: %d", x);                                  \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that a pointer is not NULL
 * @param x Pointer to test
 * @note Test returns immediately if assertion fails
 */
#define ASSERT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_TEST("ASSERT_NOT_NULL failed: %d", x);                              \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Expect that two values are equal
 * @param x First value to compare
 * @param y Second value to compare
 * @note Test continues even if expectation fails
 */
#define EXPECT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_TEST("EXPECT_EQ failed: %d != %d", x, y);                           \
		}                                                                           \
	} while (0)

/**
 * @brief Expect that two values are not equal
 * @param x First value to compare
 * @param y Second value to compare
 * @note Test continues even if expectation fails
 */
#define EXPECT_NE(x, y)                                                             \
	do {                                                                            \
		if ((x) == (y)) {                                                           \
			LOG_TEST("EXPECT_NE failed: %d == %d", x, y);                           \
		}                                                                           \
	} while (0)

#define EXPECT_GT(x, y)                                                             \
	do {                                                                            \
		if ((x) <= (y)) {                                                           \
			LOG_TEST("EXPECT_GT failed: %d <= %d", x, y);                           \
		}                                                                           \
	} while (0)

#define EXPECT_LT(x, y)                                                             \
	do {                                                                            \
		if ((x) >= (y)) {                                                           \
			LOG_TEST("EXPECT_LT failed: %d >= %d", x, y);                           \
		}                                                                           \
	} while (0)

#define EXPECT_TRUE(x)                                                              \
	do {                                                                            \
		if (!(x)) {                                                                 \
			LOG_TEST("EXPECT_TRUE failed: %d", x);                                  \
		}                                                                           \
	} while (0)

#define EXPECT_FALSE(x)                                                             \
	do {                                                                            \
		if (x) {                                                                    \
			LOG_TEST("EXPECT_FALSE failed: %d", x);                                 \
		}                                                                           \
	} while (0)

#define EXPECT_NULL(x)                                                              \
	do {                                                                            \
		if ((x) != nullptr) {                                                          \
			LOG_TEST("EXPECT_NULL failed: %d", x);                                  \
		}                                                                           \
	} while (0)

/**
 * @brief Expect that a pointer is not NULL
 * @param x Pointer to test
 * @note Test continues even if expectation fails
 */
#define EXPECT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_TEST("EXPECT_NOT_NULL failed: %d", x);                              \
		}                                                                           \
	} while (0)