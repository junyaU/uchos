---
name: kernel-testing
description: UCHos カーネル内部テストの追加・実行手順。新機能のテストを書くとき、既存テストを修正・デバッグするときに使う。
---

# カーネルテストの追加と実行

## テスト追加の全手順

1. `kernel/tests/test_cases/<対象>_test.cpp` と `.hpp` を作成:

   ```cpp
   // <対象>_test.hpp
   #pragma once
   void register_<対象>_tests();

   // <対象>_test.cpp
   #include "<対象>_test.hpp"
   #include "tests/framework.hpp"
   #include "tests/macros.hpp"

   namespace
   {
   void test_basic_behavior()
   {
       ASSERT_EQ(1 + 1, 2);
       EXPECT_EQ(2 + 2, 4);  // 失敗しても継続
   }
   }  // namespace

   void register_<対象>_tests()
   {
       test_register("test_basic_behavior", test_basic_behavior);
   }
   ```

2. `kernel/tests/test_cases/CMakeLists.txt` にソースファイルを追加

3. `kernel/main.cpp` に登録(既存の `run_test_suite` 呼び出しの並びに追加):
   ```cpp
   run_test_suite(register_<対象>_tests);
   ```

4. ビルド確認: `cmake -B build kernel && cmake --build build`

5. `./lint.sh` で警告ゼロを確認

## マクロの使い分け
- `ASSERT_*`: 前提条件。失敗したら以降のテストコードが無意味な場合に使う(即 return)
- `EXPECT_*`: 検証項目。失敗しても他の検証を続けたい場合に使う

## 実行について
- テストは QEMU 上のカーネル起動時に実行され、結果は `LOG_TEST` で画面に出る
- QEMU の起動(`./run_qemu.sh`)はユーザーが行う。Claude はビルドと lint まで
- 実行結果の確認が必要な場合はユーザーに依頼すること
