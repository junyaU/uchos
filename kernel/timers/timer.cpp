#include "timer.hpp"
#include "../graphics/log.hpp"
#include "../memory/slab.hpp"
#include "../task/ipc.hpp"
#include "../types.hpp"

uint64_t kernel_timer::calculate_timeout_ticks(unsigned long millisec) const
{
	return tick_ + (millisec * TIMER_FREQUENCY) / 1000;
}

uint64_t kernel_timer::add_timer_event(unsigned long millisec,
									   timeout_action_t action,
									   task_t task_id)
{
	auto e = timer_event{
		.id = last_id_++,
		.task_id = task_id,
		.timeout = calculate_timeout_ticks(millisec),
		.period = static_cast<unsigned int>(millisec),
		.action = action,
	};

	events_.push(e);

	return e.id;
}

uint64_t kernel_timer::add_periodic_timer_event(unsigned long millisec,
												timeout_action_t action,
												task_t task_id,
												uint64_t id)
{
	if (id == 0) {
		id = last_id_++;
	} else if (last_id_ < id) {
		printk(KERN_ERROR, "invalid timer id: %lu", id);
		return 0;
	}

	auto e = timer_event{
		.id = id,
		.task_id = task_id,
		.timeout = calculate_timeout_ticks(millisec),
		.period = static_cast<unsigned int>(millisec),
		.periodical = 1,
		.action = action,
	};

	events_.push(e);

	return e.id;
}

void kernel_timer::add_switch_task_event(unsigned long millisec)
{
	auto e = timer_event{};
	e.action = timeout_action_t::SWITCH_TASK;
	e.timeout = calculate_timeout_ticks(millisec);
	events_.push(e);
}

void kernel_timer::remove_timer_event(uint64_t id)
{
	if (id == 0 || last_id_ < id) {
		printk(KERN_ERROR, "invalid timer id: %lu", id);
		return;
	}

	ignore_events_.insert(id);
}

bool kernel_timer::increment_tick()
{
	++tick_;

	bool need_switch_task = false;
	while (!events_.empty() && events_.top().timeout <= tick_) {
		auto e = events_.top();
		events_.pop();

		if (e.action == timeout_action_t::SWITCH_TASK) {
			need_switch_task = true;

			e.timeout = calculate_timeout_ticks(SWITCH_TASK_MILLISEC);
			events_.push(e);

			continue;
		}

		message m;
		m.type = NOTIFY_TIMER_TIMEOUT;
		m.sender = KERNEL_TASK_ID;
		m.data.timer.action = e.action;
		send_message(e.task_id, &m);

		if (e.periodical == 1) {
			e.timeout = calculate_timeout_ticks(e.period);
			events_.push(e);
		}
	}

	return need_switch_task;
}

kernel_timer* ktimer;
void initialize_timer()
{
	printk(KERN_INFO, "Initializing logical timer...");

	void* addr = kmalloc(sizeof(kernel_timer), KMALLOC_ZEROED);
	if (addr == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for kernel timer.");
		return;
	}

	ktimer = new (addr) kernel_timer;

	printk(KERN_INFO, "Logical timer initialized successfully.");
}
