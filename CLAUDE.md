# UCHos 開発ガイド

## 🎯 プロジェクト概要

**UCHos** - シンプルなマイクロカーネル OS

### 基本方針
- **とにかくシンプルに** - 複雑さを排除し、理解しやすい設計
- **カーネルは積極的に薄くする** - 最小限の機能をカーネルに配置
- **システムコールは極力少なく** - 必要最小限のインターフェース

### システム仕様
- **対応アーキテクチャ**: x86_64
- **カーネルアーキテクチャ**: マイクロカーネル（ぽいもの）
- **プロセッサ**: シングルプロセッサ対応
- **最終更新**: 2025年1月26日

## 🏗️ システム構成

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
# 全体ビルド & QEMU実行（推奨）
./run_qemu.sh

# カーネルのみビルド
cmake -B build kernel && cmake --build build

# コード品質チェック
./lint.sh
```

### 便利なエイリアス
```bash
# .bashrc や .zshrc に追加推奨
alias ub='./run_qemu.sh'    # UCHos Build & Run
alias ul='./lint.sh'        # UCHos Lint
alias uk='cmake -B build kernel && cmake --build build'  # UCHos Kernel build
```

## 🧪 テスト戦略

### テスト方針
バグ修正による開発停滞を避けるため、包括的なテスト機構を構築する。

### 1. カーネル内部テスト
```cpp
// kernel/tests/ ディレクトリ構成
kernel/tests/
├── memory_tests.cpp      // メモリ管理テスト
├── process_tests.cpp     // プロセス管理テスト
├── ipc_tests.cpp         // IPC テスト
└── fs_tests.cpp          // ファイルシステムテスト

// 機能セットアップ時のテスト例
void memory_init() {
    setup_buddy_system();
    setup_slab_allocator();
    
    // 初期化後に即座にテスト実行
    run_test_suite(memory_tests);
}
```

### 2. 機能テストの実行タイミング
```cpp
// kernel/main.cpp での実行例

// 個別機能テスト（初期化時）
memory_init();     // → memory_tests 自動実行
process_init();    // → process_tests 自動実行

// 複合機能テスト（全セットアップ完了後）
run_integration_tests();

// 特定テストのみ実行する場合
run_test_suite(memory_tests);        // メモリテストのみ
run_test_suite(filesystem_tests);    // ファイルシステムテストのみ
```

### 3. ユーザーランドからのテスト
```cpp
// システムコール経由での機能テスト
// user/test_app/ でカーネル機能を検証

// 例：メモリ管理のシステムコールテスト
void test_memory_syscalls() {
    void* ptr = sys_mmap(size);
    assert(ptr != nullptr);
    sys_munmap(ptr, size);
}
```

### 4. CI自動テスト（予定）
- GitHub Actions での自動ビルド・テスト
- QEMU環境での自動実行テスト

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
- **アーキテクチャ**: x86_64
- **文字エンコーディング**: UTF-8

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
```cpp
システムコールは最小限に抑える

```

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
│   ├── process/        # プロセス管理
│   ├── ipc/            # プロセス間通信
│   ├── fs/             # ファイルシステム（FAT64）
│   ├── drivers/        # デバイスドライバ
│   │   ├── virtio_blk.cpp
│   │   └── xhci_kbd.cpp
│   └── tests/          # カーネル内部テスト
├── loader/             # ブートローダー
├── user/               # ユーザーランド
│   ├── terminal/       # ターミナルアプリ
│   ├── commands/       # コマンド群
│   └── test_app/       # テスト用アプリ
└── docs/               # ドキュメント
```

## ⚡ 開発Tips

### デバッグ
```cpp
// QEMU + GDB でのカーネルデバッグ
// ブレークポイント設定
asm volatile("int3");  // x86_64 ブレークポイント

// メモリダンプ
void dump_page_table(uint64_t cr3);
```
