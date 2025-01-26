#pragma once

#include "graphics/log.hpp"

#define ASSERT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_ERROR("ASSERT_EQ failed: %d != %d", x, y);                          \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NE(x, y)                                                             \
	do {                                                                            \
		if ((x) == (y)) {                                                           \
			LOG_ERROR("ASSERT_NE failed: %d == %d", x, y);                          \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_GT(x, y)                                                             \
	do {                                                                            \
		if ((x) <= (y)) {                                                           \
			LOG_ERROR("ASSERT_GT failed: %d <= %d", x, y);                          \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_LT(x, y)                                                             \
	do {                                                                            \
		if ((x) >= (y)) {                                                           \
			LOG_ERROR("ASSERT_LT failed: %d >= %d", x, y);                          \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_TRUE(x)                                                              \
	do {                                                                            \
		if (!(x)) {                                                                 \
			LOG_ERROR("ASSERT_TRUE failed: %d", x);                                 \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_FALSE(x)                                                             \
	do {                                                                            \
		if (x) {                                                                    \
			LOG_ERROR("ASSERT_FALSE failed: %d", x);                                \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NULL(x)                                                              \
	do {                                                                            \
		if ((x) != NULL) {                                                          \
			LOG_ERROR("ASSERT_NULL failed: %d", x);                                 \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define ASSERT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_ERROR("ASSERT_NOT_NULL failed: %d", x);                             \
			return;                                                                 \
		}                                                                           \
	} while (0)

#define EXPECT_EQ(x, y)                                                             \
	do {                                                                            \
		if ((x) != (y)) {                                                           \
			LOG_ERROR("EXPECT_EQ failed: %d != %d", x, y);                          \
		}                                                                           \
	} while (0)

#define EXPECT_NE(x, y)                                                             \
	do {                                                                            \
		if ((x) == (y)) {                                                           \
			LOG_ERROR("EXPECT_NE failed: %d == %d", x, y);                          \
		}                                                                           \
	} while (0)

#define EXPECT_GT(x, y)                                                             \
	do {                                                                            \
		if ((x) <= (y)) {                                                           \
			LOG_ERROR("EXPECT_GT failed: %d <= %d", x, y);                          \
		}                                                                           \
	} while (0)

#define EXPECT_LT(x, y)                                                             \
	do {                                                                            \
		if ((x) >= (y)) {                                                           \
			LOG_ERROR("EXPECT_LT failed: %d >= %d", x, y);                          \
		}                                                                           \
	} while (0)

#define EXPECT_TRUE(x)                                                              \
	do {                                                                            \
		if (!(x)) {                                                                 \
			LOG_ERROR("EXPECT_TRUE failed: %d", x);                                 \
		}                                                                           \
	} while (0)

#define EXPECT_FALSE(x)                                                             \
	do {                                                                            \
		if (x) {                                                                    \
			LOG_ERROR("EXPECT_FALSE failed: %d", x);                                \
		}                                                                           \
	} while (0)

#define EXPECT_NULL(x)                                                              \
	do {                                                                            \
		if ((x) != NULL) {                                                          \
			LOG_ERROR("EXPECT_NULL failed: %d", x);                                 \
		}                                                                           \
	} while (0)

#define EXPECT_NOT_NULL(x)                                                          \
	do {                                                                            \
		if ((x) == NULL) {                                                          \
			LOG_ERROR("EXPECT_NOT_NULL failed: %d", x);                             \
		}                                                                           \
	} while (0)