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

### エラー規約(2026-07-23 決定、issue #356)
- **単一体系**: エラーコードは `ErrorCode`(`libs/common/types.hpp`)のみ。カーネル内部・syscall ABI・IPC 応答のすべてで同じ負の `error_t` を使う。errno 等への変換層は作らない
- **syscall 境界**: 戻り値は RAX 一本。非負 = 成功値、負 = `error_t`(Linux の -errno 方式と同型)。境界でエラーを `-1` や `0` に潰して情報を捨てない
- **IPC は二層**: ①送達の成否 = `call()` / `send_message` の `error_t` 戻り値、②処理の成否 = 応答 `Message::result`(`OK` または負の `error_t`。payload は `OK` 時のみ有効)。要求を受けたハンドラはエラー時も必ず result にエラーを載せて reply する
- **層別**: alloc 系 = nullptr 返し(呼び出し元で必ずチェック)/ それ以外 = `error_t` / panic は「続行すると必ずメモリ・状態破壊に至る」場合のみ
- 成否判定は `IS_ERR` / `IS_OK`(`types.hpp`)に統一。新規コードで戻り値のエラーを黙って破棄しない

### ログ
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
