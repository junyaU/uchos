#pragma once

#include "graphics/log.hpp"

#define ASSERT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_TEST("ASSERT_EQ failed: %d != %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NE(x, y)                                                             \
	do {                                                                            \
		if ((x) == (y)) {                                                           \
			LOG_TEST("ASSERT_NE failed: %d == %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_GT(x, y)                                                             \
	do {                                                                            \
		if ((x) <= (y)) {                                                           \
			LOG_TEST("ASSERT_GT failed: %d <= %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_LT(x, y)                                                             \
	do {                                                                            \
		if ((x) >= (y)) {                                                           \
			LOG_TEST("ASSERT_LT failed: %d >= %d", x, y);                           \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_TRUE(x)                                                              \
	do {                                                                            \
		if (!(x)) {                                                                 \
			LOG_TEST("ASSERT_TRUE failed: %d", x);                                  \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_FALSE(x)                                                             \
	do {                                                                            \
		if (x) {                                                                    \
			LOG_TEST("ASSERT_FALSE failed: %d", x);                                 \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NULL(x)                                                              \
	do {                                                                            \
		if ((x) != NULL) {                                                          \
			LOG_TEST("ASSERT_NULL failed: %d", x);                                  \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_TEST("ASSERT_NOT_NULL failed: %d", x);                              \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define EXPECT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_TEST("EXPECT_EQ failed: %d != %d", x, y);                           \
		}                                                                           \
	} while (0)

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
		if ((x) != NULL) {                                                          \
			LOG_TEST("EXPECT_NULL failed: %d", x);                                  \
		}                                                                           \
	} while (0)

#define EXPECT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_TEST("EXPECT_NOT_NULL failed: %d", x);                              \
		}                                                                           \
	} while (0)