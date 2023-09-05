#include "task_manager.hpp"

#include "../point2d.hpp"
#include "context.hpp"
#include "graphics/screen.hpp"
#include "graphics/system_logger.hpp"
#include "memory/memory.h"
#include "memory/segment.hpp"
#include "task.h"

#include <cstdio>
#include <vector>

alignas(16) Context task_main_context, task_2_context;

// test task
void Task2()
{
	int i = 0;
	while (true) {
		char count[14];
		sprintf(count, "%d", i);

		Point2D draw_area = { static_cast<int>(300),
							  static_cast<int>(screen->Height() -
											   screen->Height() * 0.08) };

		screen->FillRectangle(draw_area,
							  { bitmap_font->Width() * 8, bitmap_font->Height() },
							  screen->TaskbarColor().GetCode());

		screen->DrawString(draw_area, count, 0xffffff);

		i++;

		ExecuteContextSwitch(&task_main_context, &task_2_context);
	}
}

void InitializeTask2Context()
{
	std::vector<uint64_t> stack(1024);
	uint64_t stack_end = reinterpret_cast<uint64_t>(&stack[1024]);

	memset(&task_2_context, 0, sizeof(task_2_context));

	task_2_context.cr3 = GetCR3();
	task_2_context.rsp = (stack_end & ~0xflu) - 8;
	task_2_context.rflags = 0x202;
	task_2_context.rip = reinterpret_cast<uint64_t>(Task2);
	task_2_context.cs = kKernelCS;
	task_2_context.ss = kKernelSS;

	*reinterpret_cast<uint32_t*>(&task_2_context.fxsave_area[24]) = 0x1f80;
}
