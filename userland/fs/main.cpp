#include <libs/common/message.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/print.hpp>
#include <libs/user/syscall.hpp>

int main(void) {
  Message msg;
  while (true) {
    receive_message(&msg);
    if (msg.type == MsgType::NO_TASK) {
      continue;
    }

    switch (msg.type) {}
  }

  return 0;
}