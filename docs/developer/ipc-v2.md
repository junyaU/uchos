# IPC v2 設計文書 (issue #314)

2026-07-19 起草。全体監査(#312〜#315)で確定した「壊れやすさの根源は IPC 設計」という結論を受け、
メッセージ基盤を作り直す。本文書はレビュー用の設計であり、実装はステージ分割で行う(§15)。

## 1. 目標(issue #314 完了条件)

1. `libs/common/message.hpp` から生ポインタが消える
2. 受信待ちタスクが CPU を消費しない(スピンなし)
3. ISR 文脈でのヒープ確保 0
4. 256 バイト制限の撤廃(任意サイズの read/write)
5. 既存のシェル・全コマンドが動作
6. (#312 移管分)`tool_desc` → `ool_desc` 改名、`MsgType` 命名統一、エラー規約の決定

## 2. 現状の問題(調査結果の要約)

| 問題 | 場所 |
|---|---|
| `sys_ipc(IPC_RECV)` が空キューで即返し → 全ユーザーランドがスピン待ち | kernel/syscall/handler.cpp:176-194, userland/shell/main.cpp:18, libs/user/ipc.cpp:17 |
| `Message` が約 1.6KB(`net.packet_data[1514]` が支配)で `std::deque` にヒープコピー、ISR からも push | libs/common/message.hpp:134, kernel/interrupt/handlers.cpp:31,57,66 |
| 型別待ち `wait_for_message` による順序すり抜け(カーネル 6 箇所+ユーザーランド) | kernel/task/task.cpp:445, handler.cpp:100,260,273,311 |
| 生ポインタ `fs.buf` / `blk_io.buf` の跨タスク所有権(確保と free が別タスク) | fat/file.cpp, virtio/blk.cpp:40,80 |
| OOL(tool_desc)は send_message 内でマップし、解放は「送信者の CR3 で unmap」という暗黙依存。物理 free は経路によって欠落 | kernel/task/ipc.cpp:17-55 |
| `temp_buf[256]` 超の write は `ERR_INVALID_ARG` 拒否 | handler.cpp:88-95 |
| FS が send_message を迂回してキューへ直挿し + 初期化前メッセージの自前棚上げ | fs/fat/init.cpp:29,64-68 |
| IRQ ハンドラに宛先 PID 直書き | handlers.cpp:31,43,57,66 |
| `send_message` の `[[gnu::no_caller_saved_registers]]` で返り値が呼び出し元で不定 | kernel/task/ipc.hpp:39-45 |
| `sys_wait` が IPC_EXIT_TASK を型待ちし SHELL へ転送するハック | handler.cpp:306-320 |

前提事実: カーネルサービス(fat32/virtio_blk/net 等)は ring0・カーネル空間共有。
ユーザープロセス(shell とコマンド)は ring3・別 CR3 で、カーネルヒープには触れない
(OOL マップのみが受け渡し手段)。

## 3. 設計概要 — 3 つのプリミティブ

通信を性質の異なる 3 種に分離する。すべての投入経路はこの API に封鎖され、キューへの直接アクセスは型で禁止する。

| プリミティブ | 文脈 | 実体 | 用途 |
|---|---|---|---|
| `notify(dst, NotifyType)` | **ISR 可** | タスク毎のビットマスクに OR + 起床 | ドアベル(xhci / virtio 完了 / タイマー)。ペイロードなし・合体(coalesce)する |
| `send(dst, Message&)` | タスクのみ | 固定長リングへ push + 起床 | 一方向メッセージ(キー入力、stdout など)。応答なし |
| `call(dst, Message&, reply)` / `reply(req, Message&)` | タスクのみ | send + 応答スロット待ち | 要求/応答 RPC。correlation id で対応付け |

受信は `receive(&msg)` の一本のみ。**ブロッキングが基本**で、保留中の通知ビット →
リング先頭の順に返す(通知は `NOTIFY_*` の合成 Message として届くため、既存のハンドラテーブル
ディスパッチ(`process_messages`)はそのまま機能する)。

- ISR は **リングにもヒープにも一切触れない**(notify のビット OR のみ)→ 完了条件 3
- 受信空 → TASK_WAITING で休眠、送信側が起床 → 完了条件 2
- `[[gnu::no_caller_saved_registers]]` は notify(返り値 void)へ移し、send/call の error_t は信頼できる値になる

## 4. Message v2

```cpp
struct OolDesc {
	uint64_t addr;   // 送信時: 送信側空間の vaddr / 受信時: 受信側空間の vaddr
	uint32_t size;   // バイト数。0 = OOL なし
	uint32_t reserved;
};

struct Message {
	MsgType type;
	ProcessId sender;
	uint32_t correlation;  // 0 = 応答不要(fire-and-forget)
	uint32_t flags;        // bit0: REPLY
	int32_t result;        // 応答の処理結果 (error_t)。要求では OK
	OolDesc ool;
	union { ... } data;    // インラインペイロード。最大 128B
};
static_assert(sizeof(Message) <= 192, "Message must stay ring-friendly");
```

- **生ポインタフィールド(`fs.buf` / `blk_io.buf` / `temp_buf[256]` / `net.packet_data[1514]`)は全廃**。
  ペイロードは「インライン ≤128B」か「OOL」の二択(§9)→ 完了条件 1
- `result` をヘッダに昇格し、全 RPC 応答のエラー表現を一本化(§11)
- `request_id` / `sequence` / `dst_type`(FS↔blk の自前対応付け)は correlation + 同期化(§14)で不要になり削除
- union 各メンバは 128B 以内: `key_input`, `init`, `memory_usage`, `fs`(fd/len/offset/name[64] 等のパラメータのみ)、
  `blk`(sector/len のみ)、`write`(短い stdout はインライン、超過は OOL)

## 5. 配送機構 — 固定長リング

```cpp
class MessageQueue {           // Task のメンバ。実体は Message ring[64] + head/tail
public:
	size_t size() const;
private:
	bool push(const Message& m);   // 満杯なら false
	bool pop(Message* out);
	friend /* kernel::task の send/receive/reply/exit 清掃のみ */;
};
```

- `std::deque<Message>` を置換。**ヒープ確保ゼロ・上限固定**(64 スロット、Message ≤192B なので約 12KB/タスク)
- push/pop は `IrqGuard` 下(通知ビット・起床判定と同一クリティカルセクション。lost-wakeup 規律は現行
  `process_messages` の cli 窓方式を踏襲)
- **満杯時: `send`/`call` は即 `ERR_QUEUE_FULL` を返す(ブロックしない)**。
  理由: 送信ブロックは相互満杯デッドロックを持ち込む。リング 64 が埋まる状況は受信側停止バグの兆候であり、
  早期に顕在化させる。ISR はリングに触れないため「ISR でのドロップ」問題自体が消滅
- private + friend でキューを封鎖。fs/fat/init.cpp の直挿しと `pending_messages` 棚上げは、
  FS 初期化の同期化(§14)で丸ごと削除

## 6. 通知ビットと IRQ ルーティング表

```cpp
enum class NotifyType : uint8_t { XHCI, VIRTIO_BLK, VIRTIO_NET_RX, VIRTIO_NET_TX, TIMER };
// Task: uint32_t pending_notifications;  // IrqGuard 下で操作
```

- `notify()` はビット OR + 起床のみ。O(1)・確保なし・冪等(連打は 1 回に合体)。
  「ドアベル喪失でサービス停止」というキュー溢れ起因の故障クラスを根絶する
- 受信時は最下位ビットから `NOTIFY_*` の合成 Message(sender = INTERRUPT)に変換して返す
- **IRQ → 宛先の登録表**: `kernel/interrupt/routing.{hpp,cpp}` に
  `register_irq_notification(vector, ProcessId, NotifyType)`。ドライバが初期化時に自分で配線し、
  handlers.cpp から特定サービスの知識(PID 直書き)を消す。未登録 vector は EOI + カウンタのみ
- タイマー: `increment_tick`(ISR)からの `send_message` を `notify(task, TIMER)` に置換。
  `TimeoutAction` ペイロードは廃止(SWITCH_TASK はカーネル内部処理のまま、メッセージ化されるのは
  カーソル点滅のみで受信側が意味を知っている)
- virtio_blk の「kick 後に完了割り込みを待つ」内部待ちは `wait_notification(mask)`(§7)で行い、
  ISR 側は `schedule_task` 直呼びから notify に統一

## 7. 待ち/起床の状態遷移

Task に待ち理由を持たせ、4 つの待ちループ(receive / call / wait_notification / sys_wait)を
1 つの `block_current(WaitState)` ヘルパに統合する。

```cpp
enum class WaitReason : uint8_t { NONE, RECEIVE, REPLY, NOTIFY, CHILD };
struct WaitState {
	WaitReason reason;
	uint32_t reply_correlation;  // REPLY 用
	uint32_t notify_mask;        // NOTIFY 用
};
```

```
RUNNING --receive() で空----------> WAITING(RECEIVE) --send/notify/reply--> READY
RUNNING --call() 送信後-----------> WAITING(REPLY)   --対応する reply-----> READY
RUNNING --wait_notification(mask)-> WAITING(NOTIFY)  --mask 内の notify--> READY
RUNNING --sys_wait で子なし-------> WAITING(CHILD)   --子の exit---------> READY
```

起床規則(すべて IrqGuard 下で判定):

| 事象 | 起床する待ち |
|---|---|
| notify(bit) | RECEIVE、または NOTIFY かつ bit ∈ mask |
| send(リング投入) | RECEIVE |
| reply(correlation 一致) | REPLY(応答スロットへ直接コピー) |
| 子タスク exit | CHILD |

- REPLY 待ち中に届いた新規要求はリングに積まれるだけで起床させない(call 完了後に処理される。
  サーバーの要求処理が自然に直列化される)
- `wait_for_message(MsgType)`(型スキャン)は**全廃**。順序すり抜け・ライブロックの根を断つ

## 8. call/reply と correlation

- `call()`: カーネルがタスク毎の単調カウンタから correlation(≠0)を採番して送信し、
  WAITING(REPLY) に入る。応答はリングを経由せず**専用の応答スロット**(Task 内 Message 1 個)に
  届く。型スキャン不要・他メッセージの順序に影響しない
- `reply(req, reply_msg)`: correlation と REPLY フラグを引き継いで req.sender へ配送。
  `req.correlation == 0`(応答不要)なら no-op。相手が REPLY 待ちでない/correlation 不一致なら
  **破棄 + LOG_ERROR**(遅延応答はプロトコルバグとして顕在化させる)
- サーバー規約: **要求(correlation ≠ 0)には成功でもエラーでも必ず 1 回 reply する**。
  結果は `result` に格納
- デッドロック規約: call の向きは非循環に限る
  (user → {KERNEL, FS}、FS → BLK、NET → なし)。サービスがクライアントへ call することを禁止
- 呼び出し側 API(カーネル・ユーザー共通の形):
  `error_t ipc_call(ProcessId dst, Message* inout)` — 送達エラーは返り値、
  アプリエラーは `inout->result`。libs/user の既存 `call()`(#344)はこの上に載せ替え

## 9. OOL 転送

**原則: ペイロードはインライン(≤128B)か OOL の二択。OOL は所有権ムーブ。**

- OOL バッファは**専用ページ**(ページ整列・ページ単位切り上げで確保)。スラブの共有ページを
  ユーザーへマップしない(無関係データの漏洩防止。現行の「slab オフセット再適用」問題も消える)
- **アドレス空間の変換は syscall 境界だけで行う**。`send_message` 内のマップ処理
  (現 `handle_ool_memory_alloc`)は廃止し、カーネル内部の配送は常にカーネルアドレスのまま:
  - **カーネル → カーネル**: アドレス無変換で所有権ムーブ。送信側は `unique_kbuf::release()` で
    手放し、受信側が `unique_kbuf` に受けて RAII(.claude/rules/kernel.md の規約どおり)
  - **ユーザー → カーネル**(sys_ipc SEND/CALL 入口): ool.addr がユーザー空間なら、カーネルが
    専用ページを確保して `copy_from_user`(上限 `OOL_MAX_SIZE` = 16MiB)。以後はカーネル所有
  - **カーネル → ユーザー**(sys_ipc RECV/CALL 出口): 受信 Message に ool があれば、その場で
    受信タスクの CR3 に user_accessible でマップし、addr をユーザー vaddr に書き換え、
    タスクの **OOL 領域表**(固定 16 エントリ: {kvaddr, uvaddr, pages})に記録
  - ユーザー → ユーザー: 上  2 つの合成で自動的に成立(コピーイン → マップ)
- 解放: `sys_ipc(…, IPC_OOL_RELEASE)` が領域表を引いて unmap + 物理 free + 表から削除。
  現行の「KERNEL 宛メッセージで dealloc(実は送信者の CR3 で unmap)」という暗黙依存を廃止
- **タスク exit 時に領域表・リング残メッセージ・応答スロットの OOL を一括解放**(現行の
  読み出し経路の物理ページリークと、異常終了時のリークを根絶)
- 満杯系: 領域表が満杯なら配送時に OOL を free し `result = ERR_OOL_LIMIT` に書き換えて配送
  (メッセージ自体は失わない)
- exec は「カーネル内 call」なので syscall 出口を通らず、ELF バッファをカーネルアドレスのまま
  受領・使用・解放する(現行の偶然動いている生ポインタ受領が正規の設計になる)

## 10. syscall インターフェース

`sys_ipc(dst, msg, flags)` の flags を拡張(新規 syscall は増やさない):

| flag | 動作 |
|---|---|
| IPC_RECV | **ブロッキング受信**(NO_TASK 即返しを廃止。syscall 内でカーネルスタック上のまま休眠) |
| IPC_SEND | 一方向送信(correlation = 0) |
| IPC_CALL | 送信 + 応答待ちを 1 syscall で。msg が応答で上書きされて返る。correlation はカーネル管理でユーザーには見せない |
| IPC_REPLY | サーバー用応答(Phase 3 のユーザーランドサービス化に必要。msg.correlation は受けた要求から引き継ぐ) |
| IPC_OOL_RELEASE | 受領済み OOL 領域の unmap + free |

- `NO_TASK` 番兵・ユーザーランドの `wait_for_message` ポーリングループ・lspci の自前スピンは全廃
- libs/user API: `ipc_recv` / `ipc_send` / `ipc_call` / `ipc_reply` / `ool_release` + 既存 `make_request`

## 11. エラー規約(#312/#356 移管分の決定)

3 層で規約化する:

1. **送達層**(`send/call/reply` の返り値・`sys_ipc` の返り値): `error_t`。
   ERR_INVALID_TASK / ERR_QUEUE_FULL / ERR_INVALID_ARG / ERR_OOL_LIMIT 等。
   「相手に届いたか」だけを表す
2. **アプリ層**(`Message::result`): サーバーの処理結果。要求には必ず応答が返り、
   成功 = OK、失敗 = 負の error_t。ペイロードの有効性は result == OK が保証
3. **syscall 層**(read/write/exec 等): 送達エラーとアプリエラーを負の error_t に畳んで返す
   (#356 Tier A で統一済みの `IS_ERR` 判定に従う)

カーネル内部の一般規約(#312 の四分裂の決着): 関数は `error_t` を返す /
確保失敗は確保箇所で ERR_NO_MEMORY に変換(`ALLOC_OR_RETURN_ERROR`)/ panic は起動不能時のみ /
void 握りつぶしは通知系(notify)のみ許容。

## 12. 子プロセス終了と wait の分離

子の終了通知を IPC から外し、カーネル内の親子管理に移す:

- Task に固定長の終了記録(`{pid, status}` × 8)を持たせ、`exit_task` は親の記録に追記 +
  親が WAITING(CHILD) なら起床
- `sys_wait` は記録から pop、空なら WAITING(CHILD) でブロック
- `IPC_EXIT_TASK`・`sys_wait` の「SHELL へ転送」ハック・shell メインループの EXIT 分岐は全廃

## 13. MsgType 改名と tool_desc → ool_desc

命名規則: **一方向通知 = `NOTIFY_*` / RPC = `<サーバー>_<操作>`**。ドアベルは MsgType から
NotifyType(§6)へ移動。

| 現行 | v2 |
|---|---|
| NOTIFY_XHCI / NOTIFY_VIRTIO_NET_RX / NOTIFY_VIRTIO_NET_TX | NotifyType へ移動 |
| NOTIFY_TIMER_TIMEOUT | NotifyType::TIMER |
| NOTIFY_KEY_INPUT / NOTIFY_WRITE | 存置(一方向通知) |
| IPC_READ_FROM_BLK_DEVICE / IPC_WRITE_TO_BLK_DEVICE | BLK_READ / BLK_WRITE |
| IPC_NET_RECV_PACKET / IPC_TRANSMIT_TO_NIC / IPC_NET_SEND_PACKET | NET_RX / NET_TX / NET_SEND |
| IPC_MEMORY_USAGE / IPC_PCI | KERNEL_MEMORY_USAGE / KERNEL_PCI_LIST(応答は OOL 配列 1 通に変更。1 要求 N 応答をやめる) |
| GET_DIRECTORY_CONTENTS | FS_LIST_DIR |
| IPC_GET_FILE_INFO / IPC_READ_FILE_DATA | FS_STAT / FS_LOAD |
| INITIALIZE_TASK | KERNEL_TASK_READY |
| IPC_EXIT_TASK / IPC_OOL_MEMORY_DEALLOC / IPC_RELEASE_FILE_BUFFER / NO_TASK / IPC_TIME | 廃止(§12 / §9-10 / 所有権ムーブで不要 / ブロッキング化で不要 / 未使用) |
| `Message::tool_desc` (MsgOolDescT) | `Message::ool` (OolDesc) |

## 14. FS・blk・net プロトコルの書き換え

- **FS ↔ blk を同期 call 化**: fat32 はクラスタ毎に `call(BLK, BLK_READ)` で応答を受け取る。
  `request_id`/`sequence` による自前の非同期対応付けと途中状態(process_read_data_response、
  pending 管理)を削除。FS 処理は直列化されるが、後続要求はリングに積まれて順に処理される
  (現行もサービスは実質 1 個ずつ処理しており、実害より状態機械の削除益が大きい)
- **FS 初期化の同期化**: BPB → FAT → ルートを `call` で順に読むだけになり、
  `pending_messages` 棚上げ・キュー直挿し・INITIALIZE_TASK 経由の再投入が全部消える
- **read 経路**: FS が応答バッファ(専用ページ)を組み → ool ムーブで reply → syscall 出口で
  ユーザーへマップ → ユーザーは使用後 `ool_release`
- **write 経路**: `sys_write` は count ≤128 ならインライン、超過ならカーネルへコピーインして
  `call(FS, FS_WRITE)`。**256B 制限撤廃**(完了条件 4)。stdout も同様(インライン or OOL)
- **net 経路**: フレームは OOL ムーブ(カーネル内なので alloc → move → 受信側 free のみ)。
  `packet_data[1514]` インラインを廃止し Message を痩身化
- keyboard の `NOTIFY_KEY_INPUT` は send のまま(送信者を INTERRUPT 詐称から XHCI に修正)

## 15. 実装ステージ(それぞれ独立にビルド・動作確認可能)

| Stage | 内容 | 達成する完了条件 |
|---|---|---|
| **A: 待ち/起床とキュー** | 固定長リング + キュー封鎖 / notify ビット + IRQ ルーティング表 / `sys_ipc(IPC_RECV)` ブロッキング化 / block_current 統合 / timer・blk の ISR 経路置換 | 2, 3 |
| **B: call/reply** | correlation + 応答スロット / IPC_CALL・IPC_REPLY / カーネル内 call(sys_write・sys_exec・shell_service)/ FS↔blk 同期化・FS 初期化同期化 / wait_for_message 全廃 / 子終了の分離(§12) | 順序破壊・ライブロック根絶 |
| **C: OOL と Message 痩身** | OOL v2(syscall 境界変換・領域表・exit 清掃)/ 生ポインタ全廃 / 256B 撤廃 / net OOL 化 / Message ≤192B / MsgType 改名・ool_desc 改名 | 1, 4, 6 |

- Stage A の時点では Message が現行サイズ(約 1.6KB)のままなので、リングは暫定 32 スロット
  (約 51KB/タスク)とし、Stage C の痩身後に 64 へ
- PR は Stage 毎に 1 本(計 3 本)。各 PR でシェル・全コマンドの QEMU 動作確認(完了条件 5)
- Message 構造はカーネル/ユーザーランド共有 ABI のため、各 Stage 内では両者を同時に更新する

## 16. テスト計画(kernel/tests/test_cases/ipc_test.cpp 新設)

- リング: FIFO 順序 / 満杯で ERR_QUEUE_FULL / 境界(64 個目)
- notify: 合成メッセージ化 / 連打の合体(2 回 notify → 受信 1 回)/ mask 待ちの選択起床
- call/reply: 対応付け / REPLY 待ち中の新規要求がリングに残る / correlation=0 への reply が no-op /
  遅延 reply の破棄
- OOL: カーネル間ムーブの所有権 / コピーイン上限 / exit 時の一括解放(ヒープデバッグのリーク検出と併用)
- 子終了: exit 先行 → wait / wait 先行 → exit の両順序
- 既存 task_test の「キュー直接操作」テストは send/receive API 経由に書き換え

## 17. 完了条件との対応

| 完了条件 | 設計上の根拠 |
|---|---|
| 生ポインタ排除 | §4(フィールド削除)+ §9(インライン/OOL 二択) |
| スピンなし受信 | §7(ブロッキング receive)+ §10(IPC_RECV ブロッキング化) |
| ISR ヒープ確保 0 | §3/§6(ISR は notify のみ、リング・ヒープ不触) |
| 256B 撤廃 | §14(write のインライン/OOL 自動分岐) |
| 既存機能動作 | §15(ステージ毎の QEMU 確認) |

## 18. 論点の決定(2026-07-19 レビュー済み)

すべて推奨案で承認された:

1. **ISR 通知の機構**: 専用ビット(notify)を採用
2. **FS↔blk の同期化**: 同期 call 化を採用(Stage B で実施)
3. **リング満杯時**: 即 ERR_QUEUE_FULL を採用(送信ブロックなし)
4. **ステージ分割**: 3 PR 分割を採用
5. 数値パラメータ: インライン上限 128B / リング 64(Stage A は暫定 32)/
   OOL 領域表 16 / OOL 上限 16MiB で確定

Stage A 実装時の追記: 起床規則の導入により、virtio-blk の「kick 後の素の sleep が
無関係な send_message で起こされ、ゼロ初期化 status(VIRTIO_BLK_S_OK == 0)を
完了成功と誤読しうる」という潜在レースが構造的に解消された(§7 の NOTIFY 待ちは
mask 一致の notify でのみ起床するため)。
