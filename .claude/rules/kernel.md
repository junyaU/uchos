---
paths:
  - "kernel/**/*.cpp"
  - "kernel/**/*.hpp"
  - "kernel/**/*.h"
---

# カーネルコードのルール

## メモリ確保
- **物理ページ**: buddy system(`kernel/memory/buddy_system.hpp`)
- **カーネルヒープ**: slab allocator の `alloc(size, flags)` / `free(addr)`(`kernel/memory/slab.hpp`)
- 確保結果は必ず nullptr チェックすること

## ヒープ所有権(RAII 必須)
- **新規・変更コードでは、所有権を持つ生ポインタ + 手動 `free()` を禁止**。所有権は `kernel::memory::unique_kbuf<T>` / `make_kbuf()`(または `std::unique_ptr`)で表現する(`kernel/memory/slab.hpp`)
- IPC などで所有権を手放す境界では `.release()` を明示的に呼ぶ。借用(見るだけ)は `.get()` を渡す
- `unique_kbuf` はデストラクタを呼ばず `free()` するだけ。生バッファか trivially destructible な型にのみ使う
- `enter_user_mode` / `exec_elf` など**戻らない呼び出しの前**ではスコープ終端に到達しないため、事前に `.reset()` か `.release()` で決着させる
- 既存コードの一括書き換えはしない。ファイルを触るタイミングで段階的に移行する(issue #322)

## エラーハンドリング
- panic は最終手段。リソース枯渇などは `LOG_ERROR` でログを出しエラーを返す
- ログは `kernel/log/log.hpp` の `LOG_DEBUG` / `LOG_INFO` / `LOG_ERROR`(printf 形式)
- 例外・RTTI は使用不可(`-fno-exceptions -fno-rtti`)

## 設計
- 新機能は「カーネルに本当に必要か?」を必ず自問する。複雑な処理はユーザーランドへ
- システムコールの追加は最小限に。追加時はユーザーへ確認を取ること
- タスク間の連携は IPC(メッセージパッシング、`kernel/task/`)を使う

## 変更後の必須作業
- `cmake -B build kernel && cmake --build build` でビルドが通ることを確認
- `./lint.sh` を実行し、警告をすべて修正
- 新機能には `kernel/tests/test_cases/` へのテスト追加
