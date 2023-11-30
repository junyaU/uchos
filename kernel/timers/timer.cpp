#include "timer.hpp"
#include "../graphics/kernel_logger.hpp"
#include "../memory/slab.hpp"
#include "../system_event_queue.hpp"
#include "../task/task_manager.hpp"

uint64_t kernel_timer::calculate_timeout_ticks(unsigned long millisec) const
{
	return tick_ + (millisec * TIMER_FREQUENCY) / 1000;
}

uint64_t kernel_timer::add_timer_event(unsigned long millisec)
{
	auto event = SystemEvent{ SystemEvent::kTimerTimeout };

	event.args_.timer.id = last_id_;
	event.args_.timer.timeout = calculate_timeout_ticks(millisec);
	event.args_.timer.period = millisec;

	events_.push(event);

	last_id_++;

	return event.args_.timer.id;
}

uint64_t kernel_timer::add_periodic_timer_event(unsigned long millisec, uint64_t id)
{
	auto event = SystemEvent{ SystemEvent::kTimerTimeout };

	if (id == 0) {
		id = last_id_;
		last_id_++;
	} else if (last_id_ < id) {
		klogger->printf("invalid timer id: %lu\n", id);
		return 0;
	}

	event.args_.timer.id = id;
	event.args_.timer.timeout = calculate_timeout_ticks(millisec);
	event.args_.timer.period = millisec;
	event.args_.timer.periodical = 1;

	events_.push(event);

	return event.args_.timer.id;
}

void kernel_timer::add_switch_task_event(unsigned long millisec)
{
	auto event = SystemEvent{ SystemEvent::kSwitchTask };

	event.args_.timer.timeout = calculate_timeout_ticks(millisec);
	events_.push(event);
}

void kernel_timer::remove_timer_event(uint64_t id)
{
	if (id == 0 || last_id_ < id) {
		klogger->printf("invalid timer id: %lu\n", id);
		return;
	}

	ignore_events_.insert(id);
}

bool kernel_timer::increment_tick()
{
	tick_++;

	if (tick_ % TIMER_FREQUENCY == 0) {
		__asm__("cli");
		events_.push(SystemEvent{ SystemEvent::kDrawScreenTimer, { { tick_ } } });
		__asm__("sti");
	}

	bool need_switch_task = false;
	while (!events_.empty() && events_.top().args_.timer.timeout <= tick_) {
		auto event = events_.top();
		events_.pop();

		if (event.type_ == SystemEvent::kSwitchTask) {
			need_switch_task = true;

			event.args_.timer.timeout =
					calculate_timeout_ticks(ktask_manager->next_quantum());

			__asm__("cli");
			events_.push(event);
			__asm__("sti");

			continue;
		}

		auto it = std::find(ignore_events_.begin(), ignore_events_.end(),
							event.args_.timer.id);
		if (it != ignore_events_.end()) {
			ignore_events_.erase(it);
			continue;
		}
		
		if (!kevent_queue->queue(event)) {
			klogger->printf("failed to queue timer event: %lu\n",
							event.args_.timer.id);
		}

		if (event.args_.timer.periodical == 1) {
			event.args_.timer.timeout =
					calculate_timeout_ticks(event.args_.timer.period);

			__asm__("cli");
			events_.push(event);
			__asm__("sti");
		}
	}

	return need_switch_task;
}

kernel_timer* ktimer;
void initialize_timer()
{
	klogger->info("Initializing logical timer...");

	void* addr = kmalloc(sizeof(kernel_timer));
	ktimer = new (addr) kernel_timer;

	klogger->info("Logical timer initialized successfully.");
}