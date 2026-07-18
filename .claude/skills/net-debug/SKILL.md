---
name: net-debug
description: UCHos のネットワークスタック(Ethernet/ARP/IPv4/ICMP, virtio-net)のデバッグ手順。パケットが届かない・ping が通らない等の調査に使う。
---

# ネットワークデバッグ

## 構成の前提

- ゲスト NIC: virtio-net-pci、MAC `52:54:00:12:34:56`
- ホスト側: TAP デバイス `tap0`(`192.168.100.1/24`)。`scripts/setup_tap.sh` が作成(要 sudo、run_qemu.sh 内で実行される)
- カーネル側の実装:
  - ドライバ: `kernel/hardware/virtio/net.cpp`(RX/TX 割り込み)
  - プロトコル: `kernel/net/`(ethernet / arp / ipv4 / icmp / checksum / packet_handler)
  - ユーザーランド: `userland/commands/ping/`

## 調査手順

1. **ホスト側でパケットキャプチャ**(ユーザーに実行を依頼):
   ```bash
   sudo tcpdump -i tap0 -n -e -v arp or icmp
   ```
   - ゲストからのフレームが見えない → ドライバ TX / virtqueue を疑う
   - フレームは出るが応答がない → ARP 解決・チェックサム・ホスト側設定を疑う
   - 応答は来るがゲストが処理しない → RX 割り込み・packet_handler を疑う

2. **TAP の状態確認**:
   ```bash
   ip addr show tap0          # 192.168.100.1/24 で UP か
   sysctl net.ipv4.ip_forward # 1 か
   ```

3. **カーネルログ**: `LOG_DEBUG` を該当パス(net.cpp の RX/TX、arp.cpp、icmp.cpp)に仕込み、
   画面出力で処理の到達点を特定する

## チェックポイント(典型的な不具合箇所)

- チェックサム: IPv4/ICMP のチェックサム計算(`kernel/net/checksum.cpp`)。バイトオーダーに注意
- ARP テーブル: 解決前の送信はキューイングされているか(`kernel/net/arp.cpp`)
- virtqueue: RX バッファの再供給、used ring のインデックス処理(`kernel/hardware/virtio/net.cpp`)
- エンディアン: ネットワークバイトオーダー(ビッグエンディアン)⇔ x86_64(リトルエンディアン)の変換漏れ

## 注意
- QEMU 起動・tcpdump 実行はユーザーが行う(sudo が必要なため)。Claude は出力の解析とコード修正を担当
