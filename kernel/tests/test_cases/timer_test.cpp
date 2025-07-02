#include "tests/test_cases/timer_test.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include "timers/timer.hpp"
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>

void test_timer_initialization() { ASSERT_EQ(kernel::timers::ktimer->current_tick(), 0); }

void test_add_timer_event()
{
	constexpr ProcessId test_task_id = ProcessId::from_raw(1);

	uint64_t event_id = kernel::timers::ktimer->add_timer_event(1000, timeout_action_t::SWITCH_TASK,
												test_task_id);
	ASSERT_NE(event_id, 0);

	uint64_t invalid_event_id =
			kernel::timers::ktimer->add_timer_event(1000, timeout_action_t::SWITCH_TASK, ProcessId::from_raw(-1));
	ASSERT_NE(invalid_event_id, 0);
}

void test_remove_timer_event()
{
	constexpr ProcessId test_task_id = ProcessId::from_raw(1);

	uint64_t event_id = kernel::timers::ktimer->add_timer_event(1000, timeout_action_t::SWITCH_TASK,
												test_task_id);
	auto err = kernel::timers::ktimer->remove_timer_event(event_id);
	ASSERT_EQ(err, OK);

	auto invalid_err = kernel::timers::ktimer->remove_timer_event(0);
	ASSERT_EQ(invalid_err, ERR_INVALID_ARG);
	auto invalid_err2 = kernel::timers::ktimer->remove_timer_event(9999);
	ASSERT_EQ(invalid_err2, ERR_INVALID_ARG);
}

void test_increment_tick()
{
	auto id = kernel::timers::ktimer->add_switch_task_event(20); // 20ms

	bool need_switch = false;
	for (int i = 0; i < 3; ++i) {
		need_switch = kernel::timers::ktimer->increment_tick();
	}

	ASSERT_FALSE(need_switch);

	kernel::timers::ktimer->remove_timer_event(id);
}

void test_tick_to_time()
{
	float time = kernel::timers::ktimer->tick_to_time(100);
	ASSERT_EQ(time, 1.0F);

	time = kernel::timers::ktimer->tick_to_time(50);
	ASSERT_EQ(time, 0.5F);
}

void register_timer_tests()
{
	test_register("timer_initialization", test_timer_initialization);
	test_register("timer_add_event", test_add_timer_event);
	test_register("timer_remove_event", test_remove_timer_event);
	test_register("timer_increment_tick", test_increment_tick);
	test_register("timer_tick_to_time", test_tick_to_time);
}
