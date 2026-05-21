# VST3Mate

アニメーションキャラクターがOSC通信を行うVST3マスコットプラグインです。  
キャラクターをクリックするとランダムにMIDIノートを生成し、DAWへ出力します。

A VST3 instrument plugin featuring an animated mascot character with OSC communication.  
Click the character to generate random MIDI notes output to your DAW.

---

## Features / 機能

- 🎵 **ランダムMIDIノート生成** — クリックするたびにスケールに沿ったノートをDAWへ出力
- 🎭 **スプライトアニメーション** — アイドル/クリック時のPNGフレームアニメーション（カスタマイズ可能）
- 📡 **OSC送受信** — ノートイベント・アニメーション状態をOSCで双方向通信
- 🔧 **JSON拡張設定** — スケール・アニメーション・OSCアドレスを外部ファイルで設定・ホットリロード
- 🎹 **複数スケール対応** — Major / Minor / Pentatonic / Blues / Whole Tone / Dorian + カスタムスケール

---

## Build / ビルド

### Prerequisites / 必要環境

- CMake 3.22+
- C++17 compiler (MSVC / Clang / GCC)
- Git

### Steps / 手順

```bash
git clone https://github.com/yourname/VST3Mate.git
cd VST3Mate
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

ビルド後のVST3ファイルは `build/VST3Mate_artefacts/Release/VST3/` に生成されます。

---

## Usage / 使い方

1. VST3ファイルをDAWのプラグインフォルダにコピーしてスキャン
2. インストゥルメントトラックに挿入
3. キャラクターをクリック → ランダムMIDIノートがDAWに出力されます
4. 「Scale」セレクターでスケールを変更
5. OSC設定でSend Host/Port・Receive Portを設定し「Apply OSC」をクリック

---

## OSC Protocol / OSCプロトコル

### 送信 (VST3Mate → 外部アプリ)

| Address         | Arguments                           | Description          |
|-----------------|-------------------------------------|----------------------|
| `/mascot/note`  | int noteNum, float vel, float dur   | ノートイベント        |
| `/mascot/state` | string state                        | 状態通知 (clicked等) |

### 受信 (外部アプリ → VST3Mate)

| Address         | Arguments                           | Description                  |
|-----------------|-------------------------------------|------------------------------|
| `/mascot/note`  | int noteNum, float vel, float dur   | ノートをトリガー              |
| `/mascot/anim`  | string animName                     | アニメーション切り替え        |
| `/mascot/state` | string state                        | 状態受信（ログ表示）          |

---

## Customization / カスタマイズ

設定ファイルは以下の場所に自動生成されます：

- **Windows**: `%APPDATA%\VST3Mate\extensions\config.json`
- **macOS**: `~/Library/Application Support/VST3Mate/extensions/config.json`
- **Linux**: `~/.config/VST3Mate/extensions/config.json`

### スプライト追加

```
extensions/
└── sprites/
    ├── idle/
    │   ├── frame_001.png
    │   ├── frame_002.png
    │   └── ...
    └── clicked/
        ├── frame_001.png
        └── ...
```

ファイル名でソートされた順にアニメーションします。  
`config.json` の `idleAnimDir` / `clickAnimDir` で使用するフォルダを切り替えられます。

### カスタムスケール追加

`config.json` の `customScales` 配列にスケールを追加：

```json
{
  "customScales": [
    {
      "name": "My Scale",
      "intervals": [0, 1, 4, 7, 10]
    }
  ]
}
```

設定ファイルの変更は2秒以内に自動で反映されます（ホットリロード）。

---

## Architecture / アーキテクチャ

```
PluginProcessor       — MIDI出力・OSC連携・設定管理のコア
├── NoteGenerator     — スケールベースのランダムノート生成
├── OscHandler        — OSC送受信 (JUCE OSCモジュール)
└── ExtensionManager  — JSON設定ファイル管理・ホットリロード

PluginEditor          — UIコンポーネント
└── CharacterComponent — スプライトアニメーション (Timer駆動)
```

---

## License

MIT License
