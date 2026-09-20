# Samples / サンプル

Run from the repository root / リポジトリ直下で実行:

```sh
gd samples/imports/main.gd
```

Output / 出力: `Hello, world!`

`imports/` uses `@import` for local scripts. `Settings.TITLE` and `Settings.USER` keep shared constants grouped without copying declarations into each script. `Greeting` exposes a static function.

`imports/`はローカルscriptを`@import`します。共通定数は`Settings.TITLE`・`Settings.USER`としてまとめて使い、各scriptに定義を複製しません。`Greeting`はstatic関数を公開します。

Package aliases use the same syntax after `gd add`; installation and version locking are described in the [package guide](https://gd-cli.progsha.com/pkg/). パッケージも`gd add`後に同じ構文で使えます。導入とversion固定は[パッケージガイド](https://gd-cli.progsha.com/pkg/)を参照してください。

`packages/` pins the public hello package in `gd.lock` and imports it with `@import hello`. 初回の取得にはネットワークが必要です。

```sh
cd samples/packages
gd install --frozen
gd main.gd
```
