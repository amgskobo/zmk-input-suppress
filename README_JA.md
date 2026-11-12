# zmk-input-suppress

全入力経路の末尾に置く小型の共通抑止器です。標準ノードを読み込み、
各経路から抑止マスクを直接渡します。プロファイル表やレイヤー監視の設定は不要です。

```dts
#include <zmk-input-suppress/input_processor_suppress.dtsi>
&trackpad_listener {
    input-processors = <&zip_absolute_to_relative>,
                       <&input_suppress (SUPPRESS_TOUCH | SUPPRESS_ABS)>;
    scroll {
        layers = <1>;
        input-processors = <&zip_absolute_to_relative>, <&scroll_mapper>,
                           <&input_suppress (SUPPRESS_TOUCH | SUPPRESS_BTN0 | SUPPRESS_ABS)>;
    };
};
```

経路の選択はZMK listenerが担当します。共通抑止器は入力元ごとに、接触中に実際に
通った経路のボタン抑止マスクを蓄積し、指を離した後の遅延クリックまで保持します。
次の新しい接触でその入力元の接触マスクだけを消します。touchの繰り返しでは消しません。
通した押下の解放は一度だけ通し、抑止した押下の解放も抑止します。
押下と解放の履歴は、新しい接触や経路変更でも途中で消しません。

別のパッドがレイヤーを上げ下げしても、この入力元のイベントがその経路を通らなければ
抑止を継承しません。各変換モジュールは共通抑止器の参照やAPIを持ちません。
[Matrix](https://github.com/amgskobo/zmk-input-matrix)の有効・無効もジェスチャーだけを制御します。全経路の末尾に同じインスタンスを置きます。

`CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES=0` は従来どおりの自動割り当てです。
各インスタンスに、有効な `zmk,input-listener` の個数分（最小1）の履歴を確保します。
1〜256を明示すると、グローバルなlistener番号0〜N-1だけを確保し、番号は詰め直しません。
上流の入力元番号は8bitなので、256で表現可能な全番号を保持します。例えば抑止器を
呼ぶのが実際の番号0と1だけなら、次を設定できます。

```conf
CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES=2
```

設定前に、コンパイル後の実番号と全ての直接呼び出し元を確認してください。
有効なlistenerと入力デバイスの基本経路・全子経路から、有効な抑止器へ範囲外の
番号で到達するとコンパイルを拒否します。上流がコンパイルする無効な子経路も検査します。
抑止器を参照しない経路は履歴不要です。無効なlistener・入力デバイスはコールバックが
ないため対象外です。直接呼び出しも同じ番号上限を守る必要があり、不正番号は従来どおり
無害化して停止し、NULL状態は入力元0を使います。

記載の32bit ARM ABIでは履歴は1入力元16バイトです。共有抑止器1個・有効listener5個で
到達する2枠を選ぶと、固定履歴は80から32バイトへ減ります。各入力元の17ボタン、
接触マスク、押下・解放の責務、フレーム状態は全て保持します。スレッド、タイマー、
ヒープ、設定は追加しません。保存キー・値は変わらず、設定消去も不要です。
高い番号を追加する前に上限を上げ、再ビルド後のDTSと実オブジェクト、実機を再確認します。
コンパイラのフレーム量はコンパイル時の証拠であり、実機のスタック・ヒープ最大使用量や
HID到着、通信の受け入れは同じイメージを導入して別途測定してください。

従来のprofile・window API、レイヤー監視、旧設定名の互換アダプターは削除しました。
旧 `amgskobo__a2r` / `amgskobo__sup` の保存値は消去しませんが読み込み・適用もしません。
抑止はDTSのマスクで決まり、Studioでの旧抑止設定は利用できません。
公開先は [zmk-input-suppress](https://github.com/amgskobo/zmk-input-suppress) です。
[abs2rel](https://github.com/amgskobo/zmk-input-abs2rel)、
[PadStick](https://github.com/amgskobo/zmk-input-padstick)、
[Matrix](https://github.com/amgskobo/zmk-input-matrix)、
[Temp Layer Touch](https://github.com/amgskobo/zmk-input-temp-layer-touch) と合わせて、
レビュー済みコミットまたはローカルのチェックアウトを選択します。
詳細なマスク・移行・制約・検証は [README.md](README.md) と [tests/README.md](tests/README.md) を参照してください。
