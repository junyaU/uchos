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
