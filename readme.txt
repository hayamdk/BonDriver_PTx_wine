□□□　概要

Linux(wine)上で動くPT3/PT1用のBonDriverです。
Windows向けに作られた録画・視聴用ソフトウェア資産のLinux上での有効活用が期待できます。


□□□　ビルド方法

・Windows(VC++)上でBonDriver_PTx_wine.dllをビルドします。

・Linux上でptx_ctrl.dllをビルドします（./build.sh）。
（開発関係、wine関係の各種パッケージのインストールが必要）

上記2つのDLLをセットで用います。


□□□　使い方

BonDriverのDLLのファイル名によって、PT1/PT3や地上波/衛星の動作モードが変わります。
BonDriver_PTx_wine.dllを使いたいモードに合わせて次のようにリネームします。

・PT3 地上波: BonDriver_PT3wine-T.dll
・PT3 衛星: BonDriver_PT3wine-S.dll
・PT1 地上波: BonDriver_PT1wine-T.dll
・PT1 衛星: BonDriver_PT1wine-S.dll


□□□　32ビット/64ビットの注意

カーネルとPTxのドライバは64ビットのものを用い、BonDriverは32ビットでビルドすることを想定しています（Windows用ソフトウェアの大半が32ビットバイナリのため）。

その想定のもと、pt3_ioctl.hにおける_IORマクロのポインタ引数をuint64_tに書き換えています。
これは、ioctlシステムコールによってカーネル空間とユーザー空間でデータをやり取りする際にカーネル側はポインタを64ビットと想定しているためで、無理やり32ビットのポインタを64ビットの整数にキャストして突っ込んでいます。

カーネル/ドライバとptx_ctrl.dllがどちらも32ビットの場合、ptx_ctrl.cにてGET_SIGNAL_STRENGTHを実行する2箇所のint*からuint64_tへのキャストを止め、pt3_ioctl.hの_IORマクロを元のものに戻す必要があります。
またカーネル/ドライバとptx_ctrl.dllがどちらも64ビットの場合はそのままでも動きますが、32ビットの場合と同様の修正を施すほうが望ましいです。なお、ptx_ctrl.dllを64ビットでビルドする場合、./build.shの-m32オプションを外す必要があります。


□□□　動作環境

以下のPT3ドライバ
https://github.com/m-tsudo/pt3/
が動作している環境でテストしています。
ほかの派生版やPT1のドライバなどもioctlのインターフェースが同じなら動くはずだと思いますが未確認です。