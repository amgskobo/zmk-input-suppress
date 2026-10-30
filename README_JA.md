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
