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
4. `kernel/main.cpp` に `run_test_suite(register_<対象>_tests);` を追加
5. ビルド確認: `cmake -B build kernel && cmake --build build`

## 注意
- テストの実行自体は QEMU 上で行われる(`./run_qemu.sh` はユーザーが実行)
- 既存テスト(memory / task / timer / virtio_blk / fs / fd / stdio)の書き方を踏襲すること
