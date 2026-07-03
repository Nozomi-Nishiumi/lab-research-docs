# 研究開発ガイド（VS Code / AI / 3D姿勢計測）

VS Code の導入・無料AI連携・3カメラによる跳躍3次元姿勢計測システムの設計と実施手順をまとめた静的サイトです。

## 収録ファイル

| ファイル | 内容 |
|---|---|
| `index.html` | 入口ページ。「環境構築」「個別研究タスク」の2カテゴリで資料を一覧(別紙は載せない) |
| `setup-vscode-ai.html` | 環境構築：VS Code 導入・AI連携 |
| `rstudio-setup.html` | 環境構築：RStudio 導入・データ可視化サンプル |
| `pose-measurement-system.html` | 個別研究タスク：カエル跳躍の3次元姿勢計測。別紙へのリンクを内包 |
| `part4-bessi-procedure.html` | 別紙：PART 04 実施手順書(カエルのページからのみ辿る) |
| `sample-visualizations.R` | RStudio用サンプルコード(任意。ローカルで使用) |

今後、環境構築には RStudio 導入など、個別研究タスクには別のタスクを、それぞれ index にカードを足す形で追加できます。

すべて単一HTML（外部依存はGoogle Fontsのみ）。相互リンク済みで、そのまま公開できます。

## GitHub Pages で公開する

### 方法1：ブラウザだけで（コマンド不要）

1. GitHub で新しいリポジトリを作成（例 `research-guide`、Public）。
2. リポジトリの「Add file」→「Upload files」で、上記3ファイルをアップロードしてコミット。
3. リポジトリの **Settings → Pages** を開く。
4. **Build and deployment → Source** を「Deploy from a branch」、**Branch** を `main` / `/ (root)` に設定して Save。
5. 数十秒〜数分後、`https://<ユーザー名>.github.io/<リポジトリ名>/` で公開されます（`index.html` が入口）。

### 方法2：git コマンドで

```bash
# ローカルの作業フォルダに3ファイルを置いた状態で
git init
git add index.html setup-vscode-ai.html pose-measurement-system.html part4-bessi-procedure.html README.md
git commit -m "Add research dev guide site"
git branch -M main
git remote add origin https://github.com/<ユーザー名>/<リポジトリ名>.git
git push -u origin main
```

push 後、上記「方法1」の手順3〜4で Pages を有効化します。

## 補足

- 公開URLは `https://<ユーザー名>.github.io/<リポジトリ名>/`。ユーザーサイト（`<ユーザー名>.github.io` という名前のリポジトリ）にすると、URLは `https://<ユーザー名>.github.io/` になります。
- 非公開にしたい場合は、Private リポジトリ + GitHub Pages（有料プランのアクセス制御）や、社内ホスティング、Netlify/Cloudflare Pages なども選べます。
- 資料内で参照している DANNCE / anipose / PyTorch3D 等のツールは更新が速いため、リンク先の現行版を随時確認してください。
