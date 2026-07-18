---
name: add-userland-command
description: UCHos に新しいユーザーランドコマンドを追加する手順。ping や echo のようなコマンドを userland/commands/ に新規作成するときに使う。
---

# 新しいユーザーランドコマンドの追加

## 手順

1. **ディレクトリ作成**: `userland/commands/<名前>/`

2. **main.cpp を作成**: 既存の類似コマンドを参考にする
   - 単純な出力系 → `userland/commands/echo/main.cpp`
   - IPC・デバイスを使う系 → `userland/commands/ping/main.cpp`, `userland/commands/lspci/main.cpp`

3. **Makefile を作成**(3行 + INCLUDES):
   ```make
   TARGET = <名前>
   OBJS = main.o
   include ../../Makefile.elf

   INCLUDES = -I./../../../
   ```

4. **ビルド確認**(x86_64-elf のパスを環境変数で指定):
   ```bash
   export CPPFLAGS="-I$PWD/x86_64-elf/include/c++/v1 -I$PWD/x86_64-elf/include -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS"
   export LDFLAGS="-L$PWD/x86_64-elf/lib"
   make -C userland/commands/<名前> clean && make -C userland/commands/<名前>
   ```

5. **git 管理の確認(重要)**: `.gitignore` は `/userland/*/*/*` を無視し `*.cpp` のみ除外している。
   **Makefile が無視されていないか `git status userland/commands/<名前>` で確認**し、
   無視される場合は `.gitignore` に除外パターン(例: `!/userland/*/*/Makefile`)を追加する。

## 登録作業は不要

`scripts/build_kernel.sh` が `userland/commands/*/Makefile` を自動検出してビルドし、
生成バイナリをディスクイメージへコピーする。シェルはファイル名でコマンドを起動する。

## 注意
- 例外・RTTI 不可(`-fno-exceptions -fno-rtti`)。newlib + libc++ の範囲で書く
- 動作確認(QEMU 起動)はユーザーが `./run_qemu.sh` で行う。Claude はビルド確認まで
