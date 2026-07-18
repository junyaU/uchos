---
paths:
  - "kernel/tests/**"
---

# カーネルテストのルール

## フレームワーク API(kernel/tests/framework.hpp)
- `test_register("name", func)` - テスト関数(`void (*)()`)を登録
- `run_test_suite(register_func)` - init → 登録 → 実行をまとめて行う。`kernel/main.cpp` から呼ぶ

## アサーションマクロ(kernel/tests/macros.hpp)
- `ASSERT_EQ` / `ASSERT_NE` / `ASSERT_TRUE` / `ASSERT_NOT_NULL` - 失敗時に即 return
- `EXPECT_EQ` - 失敗してもテスト継続
- 失敗は `LOG_TEST` で出力される

## テスト追加手順
1. `kernel/tests/test_cases/` に `<対象>_test.cpp` / `<対象>_test.hpp` を作成
2. `<対象>_test.hpp` に `register_<対象>_tests()` を宣言し、cpp 内で `test_register` を呼ぶ
3. `kernel/tests/test_cases/CMakeLists.txt` にソースを追加
4. `kernel/tests/runner.cpp` の適切なステージ(bootstrap / timer / main)に `run_test_suite(register_<対象>_tests);` を追加
5. ビルド確認: `cmake -B build kernel && cmake --build build`

## 実行方法
- テストの実行自体は QEMU 上で行われる(`./run_qemu.sh` はユーザーが実行)
- CI では `scripts/run_kernel_tests.sh` がヘッドレス実行する: `-DKERNEL_TEST_EXIT=ON` でビルドしたカーネルが isa-debug-exit(port 0xf4)へ結果を書き、QEMU 終了コード(33=PASS / 35=FAIL)とシリアルの `TEST_SUMMARY:` マーカーの両方で判定される
- 結果マーカー(`TEST_SUMMARY: total=N passed=N failed=N result=PASS|FAIL`)の形式を変える場合は `scripts/run_kernel_tests.sh` のパース処理も更新すること

## 注意
- 既存テスト(memory / task / timer / virtio_blk / fs / fd / stdio)の書き方を踏襲すること
