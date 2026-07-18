---
paths:
  - "userland/**/*.cpp"
  - "userland/**/*.hpp"
  - "userland/**/Makefile"
  - "libs/user/**"
---

# ユーザーランドコードのルール

## ビルド構成
- 各コマンドは `userland/commands/<名前>/` に `main.cpp` と `Makefile` を置く
- Makefile は共通定義 `userland/Makefile.elf` を include する最小構成:
  ```make
  TARGET = <コマンド名>
  OBJS = main.o
  include ../../Makefile.elf
  ```
- フリースタンディング環境: `-fno-exceptions -fno-rtti -mcmodel=large`、イメージベース `0xffff800000000000`
- ランタイムは `libs/user/`(start.o, syscall.o, ipc.o 等)が自動リンクされる

## 単体ビルド
`x86_64-elf/` のヘッダ・ライブラリパスが必要(`scripts/build_kernel.sh` と同じ環境変数):

```bash
export CPPFLAGS="-I<repo>/x86_64-elf/include/c++/v1 -I<repo>/x86_64-elf/include -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS"
export LDFLAGS="-L<repo>/x86_64-elf/lib"
cd userland/commands/<名前> && make clean && make
```

## 設計
- カーネルとのやり取りはシステムコール(`libs/user/syscall.o`)と IPC のみ
- 複雑なロジックはユーザーランドに置くのが UCHos の方針。カーネルに寄せない
- 新コマンドの追加手順は `/add-userland-command` スキルを参照
