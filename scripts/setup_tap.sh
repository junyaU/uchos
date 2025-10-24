#!/bin/bash

# TAPデバイスの作成
sudo ip tuntap add dev tap0 mode tap

# IPアドレス設定
sudo ip addr add 192.168.100.1/24 dev tap0

# デバイス有効化
sudo ip link set tap0 up

# パケット転送を有効化
sudo sysctl -w net.ipv4.ip_forward=1

echo "tap0 created: 192.168.100.1/24"