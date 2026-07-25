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

## ring-3 煙テスト(issue #374)
- ring 0 で完結するカーネル内テストでは fork の 2 回戻り・exec の CR3 切替・sys_wait の親子連携(#371 型回帰)を検出できない。CI はこれを `-DKERNEL_SMOKE_TEST=ON` の煙テストで補う
- 仕組み: テスト完走後もブートを継続 → シェルを `-smoke` 引数で exec → シェルが `echo smoke` を通常の fork→exec→wait 経路で 1 往復実行 → 結果を `SMOKE_REPORT` メッセージで KERNEL タスクへ報告 → カーネル(`kernel/tests/smoke.cpp`)が `SMOKE_TEST: ... result=PASS|FAIL` マーカーをシリアルに出して isa-debug-exit で終了(テスト結果との AND)。ハングは 60 秒のウォッチドッグが FAIL マーカー化する
- `scripts/run_kernel_tests.sh` は `SMOKE_BINS="<shell> <echo>"` 指定時に virtio-blk ストレージディスクを組み立てて `SMOKE_TEST:` マーカーも判定する。マーカー形式を変える場合は両側を同期すること

## 注意
- 既存テスト(memory / task / timer / virtio_blk / fs / fd / stdio)の書き方を踏襲すること
