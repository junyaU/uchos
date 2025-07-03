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
- **対応アーキテクチャ**: x86_64
- **カーネルアーキテクチャ**: マイクロカーネル（ぽいもの）
- **プロセッサ**: シングルプロセッサ対応
- **ブートローダー**: UEFI (EDK2)
- **エミュレータ**: QEMU

## システム構成

### コアカーネル機能
```
カーネル空間
├── プロセス管理
├── メモリ管理
│   ├── slab allocator
│   └── buddy system
├── プロセス間通信（IPC）
├── 割り込み処理
├── タイマー
├── ファイルシステム
│   └── FAT64
└── デバイスドライバ
    ├── virtio-blk
    └── xhci キーボードドライバ
```

### ユーザーランド
```
ユーザー空間
├── ターミナル
└── コマンド群
```

## 🚀 ビルド & 実行コマンド

### 基本コマンド
```bash
# 全体ビルド & QEMU実行（claude codeはこれを実行しない）
./run_qemu.sh

# カーネルのみビルド（claude codeはこれを実行する）
cmake -B build kernel && cmake --build build

# ユーザーランドプログラムビルド（個別）
cd userland/commands/[コマンド名]
make clean && make

# コード品質チェック (clang-tidy)
./lint.sh
```

### QEMU起動オプション
run_qemu.sh は以下の設定でQEMUを起動:
- メモリ: 1GB
- UEFI: OVMF
- USB: XHCI + キーボード
- ストレージ: virtio-blk
- デバッグ: GDBサーバー (tcp::12345)

## 🧪 テスト戦略

### テスト方針
バグ修正による開発停滞を避けるため、包括的なテスト機構を構築する。

### 1. カーネル内部テスト

#### テストフレームワーク
```cpp
// kernel/tests/framework.hpp - シンプルなテストフレームワーク
test_register("test_name", test_function);  // テスト登録
run_test_suite(test_suite_function);        // テストスイート実行

// kernel/tests/macros.hpp - アサーションマクロ
ASSERT_EQ(x, y)    // 失敗時は即座にreturn
ASSERT_TRUE(x)
ASSERT_NOT_NULL(x)
EXPECT_EQ(x, y)    // 失敗してもテスト継続
```

#### 実装済みテストケース
```
kernel/tests/test_cases/
├── memory_test.cpp      // メモリ管理テスト
├── task_test.cpp        // タスク管理テスト
├── timer_test.cpp       // タイマーテスト
└── virtio_blk_test.cpp  // VirtIO ブロックデバイステスト
```

### 2. テスト実行
```cpp
// kernel/main.cpp でのテスト実行例
run_test_suite(register_virtio_blk_tests);  // 個別テスト実行

// 新しいテストスイート追加時
void register_my_tests() {
    test_register("test_basic", test_basic_function);
    test_register("test_advanced", test_advanced_function);
}
run_test_suite(register_my_tests);
```

### 3. ユーザーランドからのテスト
将来的にユーザーランドテストアプリケーションを実装予定

## 📝 コーディング規約

### x86_64 固有の考慮事項
```cpp
// アドレス幅: 64bit
using vaddr_t = uint64_t;
using paddr_t = uint64_t;

// ページサイズ: 4KB
constexpr size_t PAGE_SIZE = 4096;

// CPU固有操作
inline void cpu_halt() {
    asm volatile("hlt");
}
```

### 基本設定
- **C++標準**: C++17
- **コンパイラ**: Clang
- **リンカー**: ld.lld
- **アーキテクチャ**: x86_64
- **文字エンコーディング**: UTF-8
- **ビルドシステム**: CMake

### フォーマット規則
```cpp
// K&R + 開きブレース後改行
if (condition) {
	do_something();  // タブインデント
}
```

### 命名規則
| 対象 | 形式 | 例 |
|------|------|-----|
| 変数・関数・名前空間 | snake_case | `page_allocator`, `get_free_pages()` |
| クラス・構造体・列挙型 | CamelCase | `PageTable`, `ProcessManager` |
| マクロ・定数 | ALL_CAPS | `KERNEL_VIRTUAL_BASE` |

### マイクロカーネル設計指針
```cpp
// ✅ カーネルは最小限の機能のみ
namespace kernel {
    void handle_syscall();      // システムコール処理
    void schedule_process();    // プロセス切り替え
    void handle_interrupt();    // 割り込み処理
}

// ✅ 複雑な処理はユーザーランドへ
// ファイルシステムの高レベル操作
// ネットワークプロトコルスタック
// グラフィックス処理
```

### システムコール設計
システムコールは最小限に抑える方針

### エラーハンドリング
```cpp
// カーネル内では panic よりログを優先
LOG_ERROR("メモリ不足: 要求サイズ {}", size);
LOG_WARN("非推奨システムコール: {}", syscall_num);
LOG_INFO("プロセス {} 開始", pid);
```

## 📁 プロジェクト構造
```
uchos/
├── kernel/              # カーネルソースコード
│   ├── main.cpp        # カーネルエントリーポイント
│   ├── memory/         # メモリ管理（buddy, slab）
│   ├── task/           # タスク管理・IPC
│   ├── file_system/    # ファイルシステム
│   ├── hardware/       # ハードウェアドライバ
│   │   ├── usb/        # USB (XHCI)
│   │   └── virtio/     # VirtIO デバイス
│   ├── interrupt/      # 割り込み処理 (IDT)
│   ├── syscall/        # システムコール
│   ├── timers/         # タイマー (ACPI, LAPIC)
│   ├── graphics/       # 画面描画・フォント
│   └── tests/          # カーネル内部テスト
├── UchLoaderPkg/       # UEFI ブートローダー
├── userland/           # ユーザーランド
│   ├── shell/          # シェル
│   ├── commands/       # コマンド群 (ls, cat, echo等)
│   └── sandbox/        # テスト用プログラム
├── libs/               # 共有ライブラリ
│   ├── common/         # カーネル・ユーザー共通
│   └── user/           # ユーザーランド用
├── x86_64-elf/         # クロスコンパイル環境
│   ├── include/        # newlib, libc++ ヘッダ
│   └── lib/            # ライブラリファイル
└── build/              # ビルド出力ディレクトリ
```

## 🏗️ アーキテクチャ詳細

### メモリ管理
- **物理メモリ**: Buddy System でページ管理
- **カーネルヒープ**: Slab Allocator (オブジェクトキャッシュ)
- **仮想メモリ**: 4レベルページング (PML4)
- **ブートストラップ**: 初期化時の一時的アロケータ

### タスク管理
- **スケジューリング**: ラウンドロビン
- **コンテキストスイッチ**: context_switch.asm
- **IPC**: メッセージパッシング方式
- **タスク状態**: 実行中/待機/スリープ

### ファイルシステム
- **フォーマット**: FAT (カスタム実装)
- **VFS**: 未実装（直接FAT操作）
- **ファイル記述子**: プロセス毎に管理

### デバイスドライバ
- **USB**: XHCI コントローラ (キーボードのみ)
- **ストレージ**: VirtIO Block Device
- **PCI**: 基本的な列挙とコンフィグレーション

## ⚡ 開発Tips

### デバッグ
```bash
# GDBでカーネルデバッグ (別ターミナルで)
gdb build/UchosKernel
target remote :12345
break KernelMain
continue
```

### ログ出力
```cpp
LOG_INFO("メッセージ");
LOG_ERROR("エラー: %s", error_msg);
LOG_TEST("テスト結果");  // テスト専用
```

### 新機能追加時の注意点
1. カーネル機能は最小限に - 複雑な処理はユーザーランドへ
2. システムコールは慎重に追加 - インターフェースは変更困難
3. テストを必ず書く - kernel/tests/test_cases/ に追加
4. lint.sh でコード品質チェック

## 📌 重要な開発指針
- **既存ファイル優先**: 新規ファイル作成より既存ファイル編集を優先
- **ドキュメント作成**: 明示的に要求された場合のみ .md ファイルを作成
- **シンプルさ重視**: UCHosの基本方針「とにかくシンプルに」を常に意識
- **マイクロカーネル**: 機能追加時は「カーネルに本当に必要か？」を再考

## 📚 コードドキュメント規約

### ドキュメントスタイル
- **言語**: 英語（Doxygenスタイル）
- **形式**: C++標準のDoxygenコメント
- **必須要素**: @file, @brief, @param, @return, @note

### ヘッダーファイルのドキュメント例
```cpp
/**
 * @file filename.hpp
 * @brief Brief description of the file's purpose
 * @date YYYY-MM-DD
 */
```

### 関数ドキュメント例
```cpp
/**
 * @brief Brief description of what the function does
 *
 * More detailed description if needed, explaining the function's
 * behavior, algorithms used, or important considerations.
 *
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 *
 * @note Any important notes or warnings
 * @example Optional usage example
 */
```

### クラス/構造体ドキュメント例
```cpp
/**
 * @brief Brief description of the class/struct
 *
 * Detailed description explaining the purpose, usage,
 * and any important design decisions.
 */
class MyClass {
    int member_;  ///< Brief description of member variable
};
```

### 列挙型ドキュメント例
```cpp
/**
 * @brief Brief description of the enum
 * @{
 */
enum MyEnum {
    VALUE1,  ///< Description of VALUE1
    VALUE2   ///< Description of VALUE2
};
/** @} */
```
