/**
 * @file main.cpp
 * @brief pongd: minimal ring-3 reference service (issue #315 3b-10)
 *
 * The first service launched from the boot manifest as a plain ELF, and
 * the template every later ring-3 service (the FS in 3b-11) follows:
 * block in receive_message, answer each request with reply_message. The
 * reply carries "pong" — a fixed transformation, so a lost reply can
 * never look like a pass to the smoke test.
 */
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/user/ipc.hpp>

int main()
{
	Message msg;
	while (true) {
		receive_message(&msg);
		if (msg.type != MsgType::PONGD_PING) {
			continue;
		}

		Message resp = make_request(MsgType::PONGD_PING);
		const char pong[] = "pong";
		memcpy(resp.data.write.buf, pong, sizeof(pong));
		resp.result = OK;
		reply_message(&msg, &resp);
	}

	return 0;
}
