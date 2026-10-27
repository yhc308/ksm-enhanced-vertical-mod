# KSM Enhanced Vertical Mod (Windows x64 専用)

**K-Shoot Mania 2.1.0を縦画面でも楽しめるように拡張する非公式Modです。**
縦画面プレイ、スキンプリセット、画面のカスタマイズ、EXスコア、HARD+APS、判定タイミング設定、一部のASIO機能を提供します。

[ダウンロード](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.2-beta2) · [ソース構成・ビルドガイド](../DEVELOPMENT.md) · [ソース全体](https://github.com/yhc308/ksm-enhanced-vertical/tree/enhanced-vertical) · [不具合報告](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues)

▶ English : **[Open](README_EN.md)**<br>
▶ 日本語 : **[開く](README_JA.md)** **(表示中)**<br>
▶ 한국어 : **[바로가기](README_KO.md)**

## 主な機能

### ◆ 縦画面プレイ

縦画面に合わせたレーンとHUDの配置でプレイできます。従来の横画面プレイにも対応しており、Vertical Optionで画面関連の設定を調整できます。
縦画面向けに用意したUI画像と追加機能も、横画面モードに対応しています。

### ◆ 判定カウンター

横画面・縦画面それぞれに合わせた配置の判定インジケーターを提供します。

### ◆ P-CRITICAL判定とEXスコア

P-CRITICAL判定、EXスコア、リザルト表示、追加記録の保存に対応しています。EX記録は既存のゲーム記録とは別に保存され、譜面のEXスコア理論値も記録されます。

### ◆ ビューポートのカスタマイズ / ユーザースキン

ゲーム画面のプレビューを見ながら、背景の明るさ、レーンやノーツの太さ、判定インジケーター、スキンなどを調整できます。

標準画像は `imgs/`、ユーザースキンは `skins/<skin name>/` から読み込みます。Viewport Customでスキンを選び、プレビューで確認できます。
スキンに不足している画像は `imgs/` の標準画像で補えるため、すべての画像を置き換える必要はありません。

### ◆ HARD+APSゲージ

HARDゲージで失敗した際にNORMALへ切り替わるAPS機能を提供します。ゲージとパーセント表示が連動して切り替わり、APSバナーには切り替え演出が適用されます。

### ◆ 判定タイミング設定

専用のテスト譜面を繰り返しプレイしながら、判定と表示の設定を調整できます。

## 不具合報告

問題が発生した場合は、[Issues](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues)にWindowsのバージョン、ゲーム・Modのバージョン、再現手順をお知らせください。スクリーンショットやエラーログも調査に役立ちます。

- ゲームログ: `<game folder>/log/session-<number>-pid<PID>/events-0.log`
- インストーラーの処理結果: `%TEMP%/KsmEnhancedVerticalSetup-last-result.json`

## クレジット・ライセンス

Codexとの共同作業で開発しました。使用している画像と音源はすべて手作業で制作されています。

原作ゲーム: [K-Shoot Mania](https://www.kshootmania.com/) / [公式ソース](https://github.com/kshootmania/ksm-v2)

- Special thanks: [KSMの開発者](https://x.com/masaka_k)
- UIスキン制作・Mod制作への貢献: [chesia_game](https://x.com/chesia_game)
- プレビュー・判定タイミング設定用の楽曲提供: [Hyung](https://open.spotify.com/artist/2soZF7u1mCALXnpwCbyGvh?si=7gvRzWqdTeqV2lEYtfmo5w)

このプロジェクトは非公式Modです。原作の著作権表記とライセンスを維持しています。
