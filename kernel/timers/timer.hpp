/**
 * @file timer.hpp
 * @brief Kernel timer management and event scheduling
 */

#pragma once

#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <queue>
#include <unordered_set>

namespace kernel::timers
{

/**
 * @brief Timer event structure for scheduled operations
 *
 * Represents a scheduled timer event that will trigger an action
 * when its timeout expires. Supports both one-shot and periodic timers.
 */
struct timer_event {
	uint64_t id;		 ///< Unique identifier for this timer event
	ProcessId task_id;	 ///< Task to notify when timer expires
	uint64_t timeout;	 ///< Absolute tick count when timer expires
	unsigned int period; ///< Period in milliseconds for periodic timers
	int periodical; ///< Flag indicating if timer is periodic (1) or one-shot (0)
	timeout_action_t action; ///< Action to perform when timer expires
};

/**
 * @brief Comparison operator for timer event priority queue
 *
 * Compares timer events by their timeout value. Note the inverted
 * comparison to create a min-heap (earliest timeout first).
 *
 * @param lhs Left timer event
 * @param rhs Right timer event
 * @return true if lhs has a later timeout than rhs
 */
inline bool operator<(const timer_event& lhs, const timer_event& rhs)
{
	return lhs.timeout > rhs.timeout;
}

/**
 * @brief Timer interrupt frequency in Hz
 *
 * Defines how many timer interrupts occur per second.
 * A value of 100 means each tick represents 10 milliseconds.
 */
static constexpr int TIMER_FREQUENCY = 100;

/**
 * @brief Main kernel timer class for managing time and timer events
 *
 * This class manages the system tick counter and a priority queue of
 * timer events. It handles both one-shot and periodic timers, and
 * provides timing services to the kernel and user processes.
 */
class kernel_timer
{
public:
	/**
	 * @brief Construct a new kernel timer
	 *
	 * Initializes the tick counter to 0 and prepares the event queue.
	 */
	kernel_timer() : tick_{ 0 }, last_id_{ 1 }
	{
		events_ = std::priority_queue<timer_event>();
		ignore_events_ = std::unordered_set<uint64_t>();
	}

	/**
	 * @brief Get the current system tick count
	 *
	 * @return uint64_t Current tick count since system startup
	 */
	uint64_t current_tick() const { return tick_; }

	/**
	 * @brief Convert tick count to time in seconds
	 *
	 * @param tick Tick count to convert
	 * @return float Time in seconds
	 */
	float tick_to_time(uint64_t tick) const
	{
		return static_cast<float>(tick) / TIMER_FREQUENCY;
	}

	/**
	 * @brief Add a one-shot timer event
	 *
	 * @param millisec Delay in milliseconds until timer expires
	 * @param action Action to perform when timer expires
	 * @param task_id Task to notify when timer expires
	 * @return uint64_t Timer event ID for later removal
	 */
	uint64_t add_timer_event(unsigned long millisec,
							 timeout_action_t action,
							 ProcessId task_id);

	/**
	 * @brief Add a periodic timer event
	 *
	 * @param millisec Period in milliseconds between timer events
	 * @param action Action to perform on each timer expiration
	 * @param task_id Task to notify on each timer expiration
	 * @param id Optional timer ID (0 to auto-generate)
	 * @return uint64_t Timer event ID for later removal
	 */
	uint64_t add_periodic_timer_event(unsigned long millisec,
									  timeout_action_t action,
									  ProcessId task_id,
									  uint64_t id = 0);

	/**
	 * @brief Add a task switch timer event
	 *
	 * Schedules a context switch after the specified delay.
	 *
	 * @param millisec Delay in milliseconds until task switch
	 * @return uint64_t Timer event ID
	 */
	uint64_t add_switch_task_event(unsigned long millisec);

	/**
	 * @brief Remove a timer event
	 *
	 * @param id Timer event ID to remove
	 * @return error_t Error code (kSuccess if removed)
	 */
	error_t remove_timer_event(uint64_t id);

	/**
	 * @brief Increment the system tick and process expired timers
	 *
	 * Called on each timer interrupt to advance the system time and
	 * trigger any timer events that have expired.
	 *
	 * @return true if any timer events were processed
	 * @return false if no events expired
	 */
	bool increment_tick();

private:
	/**
	 * @brief Calculate timeout in ticks from milliseconds
	 *
	 * @param millisec Time in milliseconds
	 * @return uint64_t Absolute tick count when timer should expire
	 */
	uint64_t calculate_timeout_ticks(unsigned long millisec) const;

	uint64_t tick_;								 ///< Current system tick counter
	uint64_t last_id_;							 ///< Last assigned timer event ID
	std::unordered_set<uint64_t> ignore_events_; ///< Set of timer IDs to ignore
	std::priority_queue<timer_event>
			events_; ///< Priority queue of pending timer events
};

/**
 * @brief Global kernel timer instance
 *
 * Single system-wide timer instance that manages all timer events.
 */
extern kernel_timer* ktimer;

/**
 * @brief Initialize the timer subsystem
 *
 * Creates the global kernel timer instance and configures the
 * timer interrupt hardware.
 */
void initialize();

} // namespace kernel::timers
