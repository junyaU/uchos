#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	Message m = make_request(MsgType::KERNEL_MEMORY_USAGE);

	Message msg = call(process_ids::KERNEL, &m);

	printu("Used memory: %u MiB\nTotal memory: %u MiB",
		   msg.data.memory_usage.used / 1024 / 1024,
		   msg.data.memory_usage.total / 1024 / 1024);

	return 0;
}
