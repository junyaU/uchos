#include "tests/test_cases/virtio_blk_test.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include <cstring>
#include <libs/common/process_id.hpp>

void test_virtio_blk_task_creation() {
  // virtio_blk_taskはinitial_tasksで自動的に作成されるため、
  // 存在確認とプロパティの検証を行う
  kernel::task::Task *t = kernel::task::get_task(process_ids::VIRTIO_BLK);
  ASSERT_NOT_NULL(t);
  ASSERT_EQ(strcmp(t->name, "virtio_blk"), 0);
}

// virtio_blk taskの初期化に実行されることが保証出来たらコメントアウトを外す
void test_virtio_blk_read_operation() {
  // // Prepare read request message
  // Message m = { .type = MsgType::IPC_READ_FROM_BLK_DEVICE,
  // 			  .sender = CURRENT_TASK->id,
  // 			  .data = { .blk_io = {
  // 								.sector = 0,
  // 								.len = 512,
  // 								.dst_type =
  // MsgType::IPC_READ_FROM_BLK_DEVICE, 						} } };

  // // Send read request
  // error_t err = send_message(process_ids::VIRTIO_BLK, &m);
  // ASSERT_EQ(err, OK);

  // // Wait for response
  // Message response = wait_for_message(MsgType::IPC_READ_FROM_BLK_DEVICE);

  // // Verify response
  // ASSERT_TRUE(response.sender == process_ids::VIRTIO_BLK);
  // ASSERT_EQ(response.data.blk_io.sector, 0);
  // ASSERT_EQ(response.data.blk_io.len, 512);
  // ASSERT_NOT_NULL(response.data.blk_io.buf);
}

// virtio_blk taskの初期化に実行されることが保証出来たらコメントアウトを外す
void test_virtio_blk_error_handling() {
  // // Test invalid sector size
  // Message m = { .type = MsgType::IPC_READ_FROM_BLK_DEVICE,
  // 			  .sender = CURRENT_TASK->id,
  // 			  .data = { .blk_io = {
  // 								.sector = 0,
  // 								.len = 100, //
  // Less than SECTOR_SIZE 								.dst_type = MsgType::IPC_READ_FROM_BLK_DEVICE, 						} } };

  // // Send request with invalid size
  // error_t err = send_message(process_ids::VIRTIO_BLK, &m);
  // ASSERT_EQ(err, OK);

  // // Wait for response
  // Message response = wait_for_message(MsgType::IPC_READ_FROM_BLK_DEVICE);

  // // Verify error response
  // ASSERT_NULL(response.data.blk_io.buf);
}

void register_virtio_blk_tests() {
  test_register("virtio_blk_task_creation", test_virtio_blk_task_creation);
  test_register("virtio_blk_read_operation", test_virtio_blk_read_operation);
  test_register("virtio_blk_error_handling", test_virtio_blk_error_handling);
}
