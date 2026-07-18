# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Please respond in Japanese.

## 🎯 プロジェクト概要

**UCHos** - シンプルなマイクロカーネル OS (x86_64 ホビープロジェクト)

### 基本方針
- **とにかくシンプルに** - 複雑さを排除し、理解しやすい設計
- **カーネルは積極的に薄くする** - 最小限の機能をカーネルに配置
- **システムコールは極力少なく** - 必要最小限のインターフェース

### システム仕様
- **アーキテクチャ**: x86_64 / シングルプロセッサ
- **ブート**: UEFI (EDK2) / **実行環境**: QEMU (OVMF, virtio-blk, virtio-net + TAP, XHCI キーボード, GDB サーバー tcp::12345)
- **言語・ツール**: C++17 / Clang / ld.lld / CMake (カーネル) + Make (ユーザーランド)

## 🚀 ビルド & 実行コマンド

```bash
# カーネルのみビルド(Claude Code はこれを実行する)
cmake -B build kernel && cmake --build build

# ユーザーランドコマンドの個別ビルド
cd userland/commands/[コマンド名] && make clean && make

# コード品質チェック (clang-tidy)
./lint.sh

# 全体ビルド & QEMU 実行(Claude Code は実行しない。settings.json で deny 済み)
./run_qemu.sh
```

### ⚠️ コード品質管理
- **コード変更後は必ず `./lint.sh` を実行し、警告が出ている場合は必ず修正すること**
- clang-format は PostToolUse フック(.claude/settings.json)で編集後に自動適用される。手動実行は不要
- すべてのテキストファイルは改行で終わること

## 📝 コーディング規約

- Google C++ Style Guide 準拠。詳細なフォーマットは `.clang-format` が唯一の正
- タブインデント、K&R + 開きブレース後改行

| 対象 | 形式 | 例 |
|------|------|-----|
| 変数・関数・名前空間 | snake_case | `page_allocator`, `get_free_pages()` |
| クラス・構造体・列挙型 | PascalCase | `PageTable`, `VirtioBlkReq` |
| マクロ・定数 | ALL_CAPS | `KERNEL_VIRTUAL_BASE` |

### エラーハンドリング
カーネル内では panic よりログを優先する(`kernel/graphics/log.hpp`、printf 形式):

```cpp
LOG_ERROR("out of memory: size %lu", size);
LOG_INFO("process %d started", pid);
LOG_TEST("test result");  // テスト専用
```

## 🏗️ マイクロカーネル設計指針

- **カーネルに実装するもの**: メモリ管理、プロセス管理、IPC、割り込み、基本的なデバイスドライバのみ
- **ユーザーランドに実装するもの**: それ以外のすべて(高レベルなファイル操作、アプリケーションロジック等)
- **機能追加時は「カーネルに本当に必要か?」を必ず再考すること**
- システムコールの追加は最小限に。インターフェースは後から変更困難

## 🧪 テスト

カーネル内部テストは `kernel/tests/` のフレームワークを使用(詳細は `.claude/rules/tests.md` が該当ファイル編集時に自動ロードされる):

```cpp
// kernel/main.cpp で実行
run_test_suite(register_my_tests);  // テストスイート登録関数を渡す
```

新機能追加時は `kernel/tests/test_cases/` にテストを必ず書くこと。

## 📁 プロジェクト構造(概要)

```
uchos/
├── kernel/
│   ├── main.cpp        # エントリーポイント & テストスイート実行
│   ├── memory/         # buddy system(物理ページ)+ slab allocator(カーネルヒープ)
│   ├── task/           # タスク管理・IPC(メッセージパッシング、ラウンドロビン)
│   ├── fs/             # FAT32(カスタム実装)、ファイル記述子
│   ├── net/            # ネットワークスタック(Ethernet / ARP / IPv4 / ICMP)
│   ├── hardware/       # PCI, XHCI キーボード, virtio(blk / net)
│   ├── interrupt/      # IDT・割り込み処理
│   ├── syscall/        # システムコール
│   ├── timers/         # ACPI, LAPIC
│   ├── graphics/       # 画面描画・フォント・ログ
│   └── tests/          # カーネル内部テストフレームワーク
├── UchLoaderPkg/       # UEFI ブートローダー(EDK2)
├── userland/           # shell/ と commands/(ls, cat, echo, ping 等)
├── libs/               # common(カーネル・ユーザー共通)/ user(ユーザーランドランタイム)
├── scripts/            # ビルド・ディスクイメージ作成・TAP 設定スクリプト
└── x86_64-elf/         # クロスコンパイル用 newlib / libc++(編集禁止)
```

最新の構造はディレクトリを直接確認すること。このツリーは概要であり網羅ではない。

## ⚡ 開発Tips

```bash
# GDB でカーネルデバッグ(QEMU 起動後に別ターミナルで)
gdb build/UchosKernel
target remote :12345
```

## 📌 重要な開発指針

- **既存ファイル優先**: 新規ファイル作成より既存ファイル編集を優先
- **ドキュメント作成**: 明示的に要求された場合のみ .md ファイルを作成
- **シンプルさ重視**: UCHos の基本方針「とにかくシンプルに」を常に意識

## 📚 コードドキュメント規約(Doxygen)

- **言語**: 英語 / **形式**: Doxygen スタイル
- **必須要素**: ファイルヘッダに `@file` `@brief`、公開関数に `@brief` `@param` `@return`
- メンバ変数は `///<` で簡潔に。*what* ではなく *why* を書く

```cpp
/**
 * @brief Allocate a physical page from the buddy system
 * @param order Allocation order (2^order pages)
 * @return Pointer to the allocated page, or nullptr on failure
 */
```

## 🔧 Claude Code 設定の構成

- `.claude/settings.json` - 共有の権限設定と clang-format 自動適用フック(コミット対象)
- `.claude/rules/` - パススコープのルール(kernel / userland / tests)。該当ファイル編集時に自動ロード
- `.claude/skills/` - 定型作業の手順書(`/add-userland-command`, `/kernel-testing`, `/net-debug`)
