# IMPLEMENT フェーズ

# ユーザの入力
#$ARGUMENTS

## 目的
plan.mdに基づきタスク単位で実装を行う。UCHosのマイクロカーネル設計原則に従った実装を確実に実行する。

## 注意事項
- 常にultrathinkでしっかりと考えて作業を行うこと
- 自明なコメントは不要
- 既存ファイルの編集を優先し、新規ファイル作成は最小限に
- マイクロカーネルの原則「とにかくシンプルに」を常に意識
- 実装後は必ず ./lint.sh を実行してコード品質を確保

## 必要な入力ファイル
- `docs/plan/plan_{TIMESTAMP}.md` - 実装計画書
- 関連する既存ファイル・コード（kernel/, userland/, libs/）

## タスクに含まれるべきTODO
1. ユーザの指示を理解し、実装開始をコンソールで通知
2. 最新の `docs/plan/plan_{TIMESTAMP}.md` ファイルを読み込み、実装計画を確認
3. プランに従った実装を段階的に実行
   - カーネル実装の場合：kernel/ディレクトリで作業
   - ユーザーランド実装の場合：userland/ディレクトリで作業
   - 共通ライブラリの場合：libs/ディレクトリで作業
4. 各実装後に必要なビルド・コンパイルを実行
   - カーネル: `cmake -B build kernel && cmake --build build`
   - ユーザーランド: 該当ディレクトリでmake
5. コード品質チェック: `./lint.sh` を実行し、警告・エラーを修正
6. テストの実装（kernel/tests/test_cases/に追加）
7. 実装内容の詳細を `docs/implement/implement_{TIMESTAMP}.md` に記録
8. 関連するplanファイル、実装詳細ファイルをコンソール出力

## UCHos実装ガイドライン
### カーネル実装
- **命名規則**: snake_case（関数・変数）、CamelCase（クラス・構造体）
- **エラーハンドリング**: panic()よりLOG_ERROR()を優先
- **メモリ管理**: Slab Allocatorを活用
- **テスト**: kernel/tests/フレームワークで必ずテストを追加

### ユーザーランド実装
- **Makefile**: Makefile.elfをインクルード
- **システムコール**: libs/user/syscall.hppを使用
- **標準ライブラリ**: newlib対応のものを使用
- **エラー処理**: 適切なエラーメッセージを表示

### コミット規則（ユーザーが明示的に要求した場合のみ）
- 小粒コミット: タスク単位で適切に分割
- コミットメッセージ: 変更内容を簡潔に記述
- テスト: 実装と同時にテストもコミット

## 実装記録ドキュメントテンプレート
```markdown
# Implementation Details: [機能名]

## 実装サマリー
[実装した機能の概要]

## プラン参照
- Plan file: docs/plan/plan_{TIMESTAMP}.md
- 実装タスク: [該当するタスク番号・名前]

## 実装詳細
### Phase 1: 基本実装
#### [タスク1名]
- ファイル: [変更したファイル:行番号]
- 変更内容:
  ```cpp
  // 実装したコードの抜粋
  ```
- 理由: [なぜこの実装方法を選んだか]

### Phase 2: テスト実装
#### カーネルテスト
- ファイル: kernel/tests/test_cases/[test_file].cpp
- テストケース:
  - test_[機能名]_basic: [基本動作テスト]
  - test_[機能名]_error: [エラーケーステスト]

### Phase 3: 品質確認
- [ ] cmake --build build 成功
- [ ] ./lint.sh エラー・警告なし

## 実装中の課題と解決
| 課題 | 解決方法 | 結果 |
|------|----------|------|
| [課題1] | [解決方法] | [結果] |

## パフォーマンス考慮事項
- メモリ使用量: [影響評価]
- 実行時間: [影響評価]
- システムコール呼び出し: [回数・頻度]

## 次のステップ
- [ ] 追加実装が必要な項目
- [ ] テストの拡充
- [ ] ドキュメントの更新
```

## 出力ファイル
- `docs/implement/implement_{TIMESTAMP}.md` - 実装詳細記録

## 最終出力形式
- 必ず以下の三つの形式で出力を行ってください

### 実装完了の場合
status: SUCCESS
next: TEST
details: "UCHos実装完了。implement_{TIMESTAMP}.mdに詳細記録。./lint.sh合格。"

### 追加作業が必要な場合
status: NEED_MORE
next: IMPLEMENT
details: "追加実装が必要。implement_{TIMESTAMP}.mdに詳細記録。タスク継続。"

### プラン見直しが必要な場合
status: NEED_REPLAN
next: PLAN
details: "設計変更が必要。implement_{TIMESTAMP}.mdに詳細記録。プランフェーズに戻る。"
