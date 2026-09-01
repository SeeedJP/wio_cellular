# Wio BG770A LTE OTA sample

WioCellularのcellular-status exampleに、LTE経由のアプリ更新を追加したサンプルです。
OTA処理は1つのライブラリ `WioOtaAgent` として取り出せます。
アプリが更新の確認時期と適用可否を決め、ライブラリが取得・検証・bootloaderへの登録を担当します。

## このサンプルでできること

[CellularStatusOta.ino](examples/CellularStatusOta/CellularStatusOta.ino)は起動時にLTEへ接続し、
SORACOM Metadataからmanifestを1回取得します。署名と配信条件を満たす新しいアプリがあれば、
Harvest Filesから取得・検証して再起動します。更新しない場合は5秒ごとに稼働ログを出します。
PlatformIOとArduino IDE／CLIは、この同じ `.ino` を使います。

初回はOTA処理を含むアプリをUSBで書き込みます。その後の更新アプリにもOTA処理と公開鍵を残してください。
このサンプルは更新を自動適用するため、初回書込み前に対象SIMグループの配信内容を確認します。

## 前提と依存関係

- Wio BG770A **HW v1.0**、対応dual-bank bootloader、SoftDevice S140 7.3.0
- SeeedJP nRF52 Arduino Core **1.5.1**、アプリ上限 **397,312 bytes**
- WioCellular **0.3.15**、ArduinoJson **7.0.4**
- SORACOM Air経由でMetadataとHarvest Filesへ接続できるSIM
- PCにPython 3、署名ツール用の `cryptography`、Ed25519対応のOpenSSL

PCへボードパッケージを入れても実機のbootloaderは更新されません。先に
[bootloaderの確認・移行手順](https://github.com/takao2704/wio_cellular/blob/873ded2438b8083a7f001cde241c6e2962187a54/extras/lte-ota-agent/docs/bootloader-migration.md)を確認してください。
この手順はforkの固定コミットに保存しています。

配布するOTAライブラリは `WioOtaAgent` の1つです。Adafruit nRFCrypto、InternalFS、TinyUSBはCore同梱版を使います。
WioCellular本体の `src/` や依存関係は変更しません。

## サンプルを試す

以降のコマンドは、このREADMEがある `extras/lte-ota` で実行します。

```bash
git clone https://github.com/SeeedJP/wio_cellular.git
cd wio_cellular/extras/lte-ota
python3 -m venv .venv
.venv/bin/python -m pip install cryptography
```

### 1. 公開鍵を用意する

通常のサンプルは両ビルド環境とも署名必須です。
生成済みの公開鍵ヘッダがなければビルドは失敗します。配布物に秘密鍵や手元の検証鍵は含めません。

既存の運用鍵があれば同じ公開鍵と `key_id` を使います。
次の鍵生成は**新規の検証用端末だけ**で行い、以後の更新でも同じ鍵を使ってください。

```bash
ota_key_dir="$(mktemp -d "$HOME/wio-ota-keys.XXXXXX")"
openssl genpkey -algorithm ED25519 -out "$ota_key_dir/signing-key.pem"
chmod 600 "$ota_key_dir/signing-key.pem"
openssl pkey -in "$ota_key_dir/signing-key.pem" -pubout \
  -out "$ota_key_dir/public-key.pem"

.venv/bin/python tools/export_manifest_public_key.py "$ota_key_dir/public-key.pem" \
  --key-id demo-key \
  --output examples/CellularStatusOta/ota_manifest_public_key.h
```

`ota_key_dir` の実際の保存先を控えてください。このシェル変数は同じセッション内でのみ有効です。
秘密鍵をスケッチ、端末、Harvest Filesへ置かないでください。本番鍵はアクセス制御した署名環境で管理します。
スケッチを別の場所へコピーした場合は、公開鍵ヘッダの出力先もそのフォルダへ変更します。

### 2-A. PlatformIOでビルドする

```bash
pio run -d examples/CellularStatusOta -e initial
pio run -d examples/CellularStatusOta -e update
```

`initial` は `APP_VERSION=1`、`update` は `APP_VERSION=2` です。
これは新規端末用のサンプル設定で、OTAプロトコルの版番号ではありません。
既存端末では現在・最高適用バージョンより大きな値へ変更します。記録を消して旧版を通さないでください。

[platformio.ini](examples/CellularStatusOta/platformio.ini)は
`WioOtaAgent=symlink://../..` でこのライブラリを参照し、共通スケッチを直接ビルドします。
別のC++ラッパーはありません。

最初のアプリだけUSBで書き込みます。既存アプリが置き換わる操作です。
対象ポートを確認し、次のプレースホルダーを置き換えて実行します。

```bash
pio run -d examples/CellularStatusOta -e initial -t upload --upload-port <WIO_PORT>
```

更新用ZIPは `examples/CellularStatusOta/.pio/build/update/firmware.zip` です。
更新先をUSBで書き込まず、手順3へ進みます。

### 2-B. Arduino IDE／CLIでビルドする

Arduino IDEの「設定」→「追加のボードマネージャのURL」に
`https://www.seeed.co.jp/package_SeeedJP_index.json` を追加し、SeeedJP nRF52 Boards 1.5.1を導入します。
既存のURLは残してください。ライブラリマネージャではWioCellular 0.3.15とArduinoJson 7.0.4を導入します。

OTAサンプルからArduino用のライブラリZIPを1つ作ります。

```bash
python3 tools/package_arduino_library.py
```

「スケッチ」→「ライブラリをインクルード」→「.ZIP形式のライブラリをインストール」で
`dist/arduino/WioOtaAgent.zip` を選択します。リポジトリ全体のZIPを読み込む形式ではありません。
旧構成の `WioOta`、`WioBg770aHttp`、`WioOtaAgent` を導入済みなら、保存先とバックアップを確認して入れ替えます。
同名ヘッダを持つ旧ライブラリと併用しないでください。

リポジトリの `examples/CellularStatusOta/CellularStatusOta.ino` を開きます。
「スケッチ例」→「WioOtaAgent」→「CellularStatusOta」からコピーしても利用できますが、
手順1の公開鍵ヘッダをコピー先にも生成する必要があります。

| ツールメニュー | 設定 |
|---|---|
| ボード | Wio BG770A |
| Board Version | 1.0 |
| SoftDevice | S140 7.3.0 |
| Print Port | Serial (USB-CDC) |
| ポート | 対象Wioのポート |

`ota_sketch_config.h` の `APP_VERSION=1`、`WIO_OTA_ARDUINO_SECURE=1` を確認し、
「検証・コンパイル」の後「書き込み」で最初のアプリをUSB導入します。
DFUへ入れない場合だけRESETをダブルクリックし、ポートを選び直します。
ここでは「ブートローダを書き込む」は実行しません。

次に `APP_VERSION` を2へ変更し、「スケッチ」→「コンパイル済みのバイナリをエクスポート」を実行します。
更新用ZIPは `examples/CellularStatusOta/build/SeeedJP.nrf52.wio_bg770a/CellularStatusOta.ino.zip` です。
公開鍵と署名必須設定は変更しません。

CLIでは上記Coreと依存ライブラリを導入したうえで実行します。
`--library .` は、このソースをライブラリとして使う指定です。

```bash
arduino-cli compile \
  --fqbn SeeedJP:nrf52:wio_bg770a:board_version=1_0,softdevice=s140v7,debug_output=serial \
  --library . --build-property compiler.cpp.extra_flags=-DAPP_VERSION=2 \
  --output-dir dist/arduino-update examples/CellularStatusOta
```

CLIの更新用ZIPは `dist/arduino-update/CellularStatusOta.ino.zip` です。

### 3. 配信ファイルと署名付きmanifestを作る

PlatformIOの更新用ZIPを使う例です。Arduinoでは入力ZIPを手順2-Bのパスへ置き換えます。

```bash
.venv/bin/python tools/firmware_manifest.py \
  examples/CellularStatusOta/.pio/build/update/firmware.zip \
  --version 2 \
  --url http://harvest-files.soracom.io/wio-bg770a/sample-update/firmware.bin \
  --signing-key "$ota_key_dir/signing-key.pem" \
  --key-id demo-key --release-id sample-update --rollout 10000 \
  --output dist/sample-update/manifest.json \
  --firmware-output dist/sample-update/firmware.bin
```

入力はapplication用DFU ZIPまたはapplication raw binaryです。
HEX、ELF、bootloaderやSoftDeviceを含むDFU ZIPは使いません。
`--version` はビルド済みの `APP_VERSION` に合わせます。ツールは版番号をバイナリから自動取得しません。
DFU ZIPの `application_version` とは別の値です。

`rollout` は0〜10000で、2500なら25%、10000なら全対象端末です。
端末IDと `release_id` から対象を決めます。同じリリースIDで配信率を増やすと対象集合が増えます。
URL・version・rolloutなど署名対象の値を変更するときは、JSONを直接編集せず再署名します。
生成ツールは端末側と同じく `http://`、host 127 bytes以下、path 255 bytes以下を受け付けます。
`--firmware-output` は `--output` と併用し、署名・検証・両ファイルの準備が終わってから置き換えます。
途中で置換に失敗した場合は、既存の2ファイルを元に戻します。

### 4. SORACOMから配信する

試験用SIMグループでMetadataを有効・read-onlyにし、Harvest Filesを有効にします。
サービス利用料金が発生します。対象SIMとグループを確認してから設定してください。
OTA実行中は通信可能な状態を維持し、PSMからの復帰と再移行はアプリ側で管理します。

1. `dist/sample-update/firmware.bin` をHarvest Filesの
   `/wio-bg770a/sample-update/firmware.bin` へアップロードし、サイズ・内容を確認します。
2. 同じ生成処理の `manifest.json` を対象グループのMetadataユーザーデータへ設定します。
3. 最初のアプリを通常リセットし、シリアルモニタを115200 bpsで開きます。DFUモードにはしません。
4. 更新前の版、ダウンロード進捗、検証、再起動後の `app-version=2`、
   同じmanifestに対する `no update` を確認します。

配信パスはリリースごとに新しく作り、公開済みのバイナリを上書きしないでください。
少数の端末で確認してから対象を広げます。Metadata更新は端末へのプッシュ通知ではありません。
このサンプルでは次の起動時確認まで更新されません。

取得先は `http://metadata.soracom.io/v1/userdata`、
許可するfirmwareホストは `harvest-files.soracom.io`、ポートは80です。
別ホストやHTTPSのURLは拒否します。固定IPではなくFQDNを設定しています。

## 既存アプリへ組み込む

PlatformIOではプロジェクトの `lib/WioOtaAgent` へ
`src/`、`scripts/`、`library.json`、`library.properties`、`LICENSE.txt` をコピーするか、
`lib_deps` に `WioOtaAgent=symlink:///absolute/path/to/lte-ota` を指定します。
Arduino IDEでは手順2-Bの1つのZIPを使います。

PlatformIOのボード・Core・サイズ上限は付属 `platformio.ini` に合わせ、
`lib_archive = no` と `-DWIO_OTA_ENABLE_ED25519` を設定します。
後者は署名検証実装のリンク指定であり、署名を必須にする設定とは別です。
Arduino IDE／CLIではnRF52840向け署名実装が自動的にビルドされます。

[共通スケッチ](examples/CellularStatusOta/CellularStatusOta.ino)の `setup()`、`checkOta()`、
`decideUpdate()` を組み込みの基準にしてください。

- アプリでLTE接続とPDP contextを準備してから `Agent::check()` を呼びます。
- `Config` で対象HW、manifest取得先、許可するfirmwareホスト・ポートを指定します。
- 通常運用では `security.require_signature=true` と公開鍵・key IDを設定します。
  アンチロールバックと段階配信も使う場合は、現在の版と永続化した最高適用版、安定した端末IDを渡します。
- `Agent` は大きなmanifestバッファを保持するため静的領域へ置きます。
  設定は生成時にコピーされるので、後から外側の `Config` だけを書き換えても反映されません。
  設定から参照する文字列や公開鍵は、Agentを使う間ずっと有効な領域へ置きます。
- アプリの判定コールバックは技術的検証・セキュリティ判定の後に呼ばれます。

| コールバックの戻り値 | 動作 |
|---|---|
| `Decision::kNoUpdate` | 更新なし。取得しない |
| `Decision::kReject` | アプリの条件で対象外。取得しない |
| `Decision::kDefer` | 端末状態などにより延期 |
| `Decision::kDownloadAndVerify` | Bank 1へ取得・検証した後、適用登録せず破棄 |
| `Decision::kApply` | 取得・検証後、モデム停止・適用登録・再起動 |

`APP_VERSION` はサンプルの識別方法で、ライブラリ必須のビルドフラグではありません。
更新判定はアプリ内定数や適用済みリリースIDなどで実装できます。
ただしアンチロールバックを使う場合は数値の現在バージョンと最高適用バージョンを正しく管理します。

日次確認や再試行もアプリの責任です。例えば起動時と24時間ごとに確認し、
`Result::kFailed` と `Result::kDeferred` は1時間後に再試行します。
連続稼働中の間隔には `millis()` の差分、再起動をまたぐ時刻指定にはRTC等と永続化した最終確認時刻を使います。
このライブラリ自体にスケジューラはありません。

`check()` は同期・非再入APIです。同じモデムを別処理から同時に操作せず、
時間制約のある処理を先に止め、SoftDeviceを無効化できる状態で呼びます。
更新適用時は戻らず再起動します。それ以外の結果ではアプリ側でモデムやPSMの通常設定へ戻します。
UFS保存の完了が90秒を超えた場合は、遅れて届く完了応答を45秒まで排出してから一時ファイルの削除を試みます。
HTTP設定、GET/READ、ファイルopen/close/deleteのタイムアウトや本文処理の中断では、
遅延応答や未読データを次回のATコマンドへ混入させないため、
Agentは戻る前にセルラーモジュールを停止します。その場合は `lastHttpRecoveryAttempted()` がtrueになり、
次のOTA確認前にアプリ側でセルラーモジュールの起動、LTE接続、PDP有効化をやり直す必要があります。
停止にも失敗した場合は `lastHttpRecoveryFailed()` がtrueとなり、`lastError()` は
`kModemPowerOffFailed` を返します。失敗時の詳細は `lastHttpError()`、`lastHttpCleanupError()`、
`lastWriterError()`、`lastSecurityError()` から取得できます。

## セキュリティと制約

署名付きmanifestのSHA-256と取得イメージを照合し、検証完了後にのみ適用登録します。
本文全体の検証はBank 1への書込み後に完了するため、未検証データが一時的に保存されることはあります。
CRC16とSHA-256だけの未署名format 1は破損検出用であり、配信者認証には使えません。
署名必須または段階配信を有効にした運用ではformat 2が必要です。

`VersionStore` はInternalFSへ最高適用版を保存し、新アプリの `setup()` 到達後に更新します。
業務処理の正常性を保証するヘルスチェックではありません。
保存領域は自動フォーマットしません。マウント済み領域に記録が両方ない場合は新規として開始し、
片方だけ正常ならその版から復旧します。既存記録があるのに正常なものが一つもない場合や、
領域をマウントできない場合はエラーを返し、サンプルはOTAを実行しません。
`begin()` に失敗したまま `recordCurrentVersion()` を呼んでも書き込みません。

初めて使う端末でInternalFSが未初期化なら、初回USB導入時に保存領域を準備してください。
Coreの `InternalFS.begin()` はマウント失敗時に領域全体を消去・初期化するため、
既存ファイルも過去の適用履歴もないと確認した新規端末の初回準備にだけ使います。
通常のOTAアプリやエラー処理には追加しないでください。
稼働済み端末でエラーが出た場合は、記録を消して再開せず、保存データと既知の適用版を確認して復旧します。
既存アプリとInternalFSを共有する場合は、操作の同時実行、記録ファイル名の衝突、
他の処理によるフォーマット・記録削除を避けてください。削除済みの履歴をライブラリが復元することはできません。
鍵の更新では、旧鍵で署名する更新アプリへ新しい公開鍵を入れ、その展開を確認してから配信側の鍵を切り替えます。
現在のConfigが受け付ける公開鍵は1つです。

通信はHTTP・固定Content-Lengthが前提で、HTTPS、chunked transfer、途中再開には未対応です。
HTTP本文は暗号化されません。再試行では先頭から取得します。
署名検証はアプリ内Agentの機能で、bootloaderのsecure bootではありません。
USB DFU／SWDによる書換えや秘密鍵漏えいは保護範囲外です。
起動できない更新アプリからの自動ロールバック、更新結果の遠隔通知、端末群の配信管理画面も含みません。

## 保守と検証

`src/` にmanifest・更新制御、HTTP/UFS転送、Bank 1書込みの実装をまとめています。
公開ヘッダ名とC++ APIは維持していますが、独立した3パッケージとしては配布しません。
`tools/` は配布ファイル生成用です。ホスト回帰テスト、明示的な実機停止試験設定と実行手順は、
[contributor forkの固定コミット](https://github.com/takao2704/wio_cellular/tree/3ba27170f6aea24827b5798a7fbef9f112bd8142/extras/lte-ota/tests)に保存しています。
本家向けツリーには、利用時に必要なサンプル、ライブラリ本体と配布ツールだけを含めます。

保存記録の破損時処理を修正した単一ライブラリ構成を、HW v1.0の1台で再検証しました。
稼働中のアプリ版9からArduino CLI生成の版10へ、その版10からPlatformIO生成の版11へ、
USBでアプリを書き戻さずにLTE OTAしています。いずれも署名付きmanifestとサービスFQDNを使い、
全量取得・イメージ検証・再起動・新アプリ起動・更新なし判定を確認しました。

事前レビュー後のHTTP/UFS回復処理を含むArduino CLI生成の版12（174,400 bytes、
SHA-256 `d875549002dc742b201482bdc5719dca8fad9f5da6a39f82a061dbf4f0ff708c`）も、
版11からLTE OTAしました。Harvest Filesへ置いたイメージの読戻し一致を確認してから署名済みmanifestを公開し、
版12の起動、`current=12`・`highest=12` の読戻し、同じmanifestの更新なし判定を確認しています。
さらに通常RESET後も `loaded=12`・`current=12`・`highest=12` が維持され、再ダウンロードされませんでした。

版11の初回取得では、モデム内ファイルへの保存が設定した90秒を超えて失敗しました。
このとき更新は適用されず、通常RESET後の再試行で取得・適用に成功しています。
版11の最終RESET後は、保存済み版11、実行中の版11、最高適用版11を読み戻し、同じmanifestを更新なしと判定しました。
また、版10への更新直後にはPDPコンテキスト取得がタイムアウトし、WioのRESETではBG770Aがリセットされないため、
USB電源の入れ直しで復旧しました。アプリ側で短期再試行やモデム復旧を設計する際の運用上の注意点です。
版12配信の初回も、版11側のモデム内ファイル保存がタイムアウトして安全に終了しました。
通常RESETだけでは残ったモデム状態からLTE初期化できなかったため、USB電源を入れ直して再試行し、更新に成功しています。
結果の要約は[PR #64](https://github.com/SeeedJP/wio_cellular/pull/64)に記載しています。
破損記録の検出、延期、取得検証のみ、失敗後の再利用は、
[forkに保存したPC上の回帰テスト](https://github.com/takao2704/wio_cellular/tree/3ba27170f6aea24827b5798a7fbef9f112bd8142/extras/lte-ota/tests/native)で確認しています。

旧構成での[実機検証記録](https://github.com/takao2704/wio_cellular/blob/873ded2438b8083a7f001cde241c6e2962187a54/extras/lte-ota-agent/docs/validation/README.md)はforkの固定コミットに保存しています。
Arduino IDE GUIのコンパイル・USB書込みや障害試験は、この過去の記録と区別してください。
日付付きの実測記録や生ログは本家向けツリーには含めません。
新構成で実機の破損レコード注入や電源断試験は実施していません。
GUI生成バイナリ同士のLTE OTA、HW v1.1、多台数の段階配信、
bootloader反映・settings・不揮発レコード書込み中の電源断は未検証です。
