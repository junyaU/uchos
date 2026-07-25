/**
 * @file services.hpp
 * @brief Boot-time service manifest access
 */

#pragma once

#include <cstddef>
#include "task/task.hpp"

namespace kernel
{

/**
 * @brief Get the boot-time service manifest
 *
 * The manifest is the single place that knows which services UCHos
 * starts and in which task slots they live (issue #315): the scheduler
 * (kernel/task) starts whatever list it is handed and knows no
 * individual service.
 *
 * @param out_count Set to the number of manifest entries
 * @return Pointer to the manifest array
 */
const task::InitialTaskInfo* service_manifest(size_t* out_count);

} // namespace kernel
