[SCE CONFIDENTIAL DOCUMENT]
PlayStation(R)3 Programmer Tool Runtime Library 400.001
                  Copyright (C) 2006 Sony Computer Entertainment Inc.
                                                 All Rights Reserved.


sce-cgcdisasm で出力したヘッダを使ってシェーダ変数の設定を行う


<サンプルのポイント>
　　このサンプルは sce-cgcdisasm を使ってパラメータ情報を出力したヘッダ
    を利用することで cellGcmCg* 系関数を呼び出さずにシェーダプログラムと
　　やり取りを行うことを示すサンプルです。サンプル内では、バーテクス
　　プログラムに対しては定数の受け渡し、頂点属性情報の取得を、フラグメント
　　プログラムに対しては定数の受け渡し、テクスチャサンプラ情報の取得といった
　　処理を行っています。
　　また本サンプルでは上記の仕組みを使うことで、SPU を使ってフラグメント
　　プログラムのパラメータを書き換える処理を実装しています。

<サンプルの解説>
　　cellGcmCgGetNamedParameter()とcellGcmCgGetParameterResource()を使って
　　バーテクスプログラムから取得していたリソース情報は sce-cgcdisasm によって
　　生成される vpshader_params.h 内で記述されている vpshader_params 構造体
　　配列から代わりに取得することができます。例えばバーテクスプログラム内で 
　　position という名前で定義されている変数は CELL_GCM_vpshader_params_position
    を使って vpshader_params[CELL_GCM_vpshader_params_position].res として
    アクセスすることができます。これから頂点属性のベース値である CG_ATTR0 を
　　引くことによって以下のように cellGcmSetVertexDataArray() を呼び出す際に必要な
　　index を得ることができます。

    cellGcmSetVertexDataArray(
        vpshader_params[CELL_GCM_vpshader_params_position].res-CG_ATTR0
        0,
        sizeof(Vertex),
        3,
        CELL_GCM_VERTEX_F,
        CELL_GCM_LOCATION_MAIN,
        getVertexOffset());

　　同様に通常cellGcmCgGetNamedParameter()とcellGcmCgGetParameterResource()で
　　取得していたテクスチャサンプラ情報も代わりにフラグメントプログラムのリソース
　　情報をヘッダから取得することで行えます。例えば本サンプルのようにフラグメント
　　プログラム内で texture という変数名で定義されているものに関しては
　　fpshader_params[CELL_GCM_fpshader_params_texture].res として取得し、実際に
　　用いるテクスチャ設定変数に渡す際にベース値を引くことで以下のように処理できます。

　　setGcmTexture(fpshader_params[CELL_GCM_fpshader_params_texture].res-CG_TEXUNIT0);

    -a オプションを使って sce-cgcstrip でシェーダバイナリをストリップした場合、
　　バイナリ内にある全てのパラメータ情報が、使用されるされないに関わらず削除
　　されます。バーテクスプログラムの場合、cellGcmSetVertexProgram() 時に設定
　　していた初期値情報も削除されるため、シェーダ内で使用する初期値をアプリケーション
　　で設定する必要があります。パラメータが初期値を持つかどうかは CellGcm_vpshader_params_Table
    の dvindex に -1 以外の値が入っているかどうかで判別することができます。初期値を
　　持つ場合、初期値情報は vpshader_params_default_value 構造体配列内で保持され、
　　dvindex はその初期値情報へのインデックスとなります。例えばバーテクスプログラム
　　内の modelViewProj[0] が初期値を持つかどうかは
    vpshader_params[CELL_GCM_vpshader_params_modelViewProj_0].dvindex
    が -1 かどうかで判別することができます。本サンプルでは 0 となっているため
　　初期値を保持し、そのインデックスが 0 なので、初期値は
　　vpshader_params_default_value[0] に保持されている値、つまり
　　{1.000000f, 0.000000f, 0.000000f, 0.000000f}
　　となります。初期値の設定は各パラメータが初期値を持っているかどうか判別し、
　　持つものに関しては cellGcmSetVertexProgramConstants() を用いて以下のように行う
　　ことが可能です。

    for (uint32_t i = 0; i < sizeof(vpshader_params)/sizeof(CellGcm_vpshader_params_Table); i++) {
        if (vpshader_params[i].dvindex != -1) {
            cellGcmSetVertexProgramConstants(
                    vpshader_params[i].resindex,
                    4,
                    vpshader_params_default_value[vpshader_params[i].dvindex].defaultvalue);
        }
    }

    またマトリクスのように連続したリソースインデックスを持つ特定のパラメータを1度に
　　更新する際は

　　cellGcmSetVertexProgramConstants(vpshader_params[CELL_GCM_vpshader_params_modelView_0].resindex, 16, M);

　　というように行うこともできます。

　　フラグメントプログラムで使用する uniform パラメータに値を渡す場合、RSX(R)では
　　フラグメントプログラム内のマイクロコード領域内に定義されている uniform パラメータ
　　用領域に直接値を書き込む必要があります。通常は cellGcmSetFragmentProgramParameter()
　　などでこれを行っているのですが、アプリケーション側でこれを制御するためには、値を
　　書き込む先のアドレスを知る必要がでてきます。sce-cgc にてコンパイルされたフラグメント
　　プログラム内にはそのアドレス情報がマイクロコードの先頭アドレスからのオフセット値という
　　形で記述されており、sce-cgcdisasm はその情報を取り出して構造体配列に格納することで
　　アプリケーションが uniform パラメータの設定を直接行うことを可能にしています。

　　上述のオフセット値情報は fpshader_params_const_offset 構造体配列で保持されており、
　　構造体配列である fpshader_params の ecindex メンバが -1 以外のパラメータがオフセット
    情報を持っていることになります。本サンプルの場合フラグメントプログラム内パラメータの
　　light がそれに当たり、fpshader_params[CELL_GCM_fpshader_params_light].ecindex が
　　fpshader_params_const_offset へのインデックスとなり、
　　fpshader_params_const_offset[fpshader_params[CELL_GCM_fpshader_params_light].ecindex].offset
　　とすることでオフセット情報にアクセスすることができます。オフセット情報の数はその
　　パラメータがフラグメントプログラム内で参照される頻度に依存しますので、可変数となります。
　　そのため 0xffffffff をオフセット情報の終端識別子として定義しており、インデックスで
　　指定された値から 0xffffffff が来るまでがそのパラメータのオフセット情報ということに
　　なり、light の場合は 0x10 のみがオフセットということになります。
　　実際に書き換える先のアドレスはマイクロコードの先頭アドレスとオフセットを足したものに
　　なるため以下のようにして const_addr を求めて、SPU に書き換え先のアドレスを通知しています。

　　uint32_t const_addr =
　　        getFpUcode() + fpshader_params_const_offset[fpshader_params[CELL_GCM_fpshader_params_light].ecindex].offset;
    setupSpuSharedBuffer(
            const_addr,
            (uint32_t)mainMemoryAlign(128, 128),
            (uint32_t)cellGcmGetLabelAddress(64)
            );

　　本サンプルにおける RSX(R) と SPU の同期は以下のような流れで行っています。

　　1. RSX(R) から SPU に対してパラメータ更新 OK の通知
　　2. SPU が通知を受け取り、RSX(R) からの通知領域をクリアして、更新するパラメータのアドレスに対して書き込みを実行
    3. SPU から RSX(R) に対してパラメータ更新終了の通知
　　4. RSX(R) は SPU からの通知を受け取りフラグメントプログラムをセットし、描画開始
　　5. 描画終了後 RSX(R) は SPU との同期領域をクリアして、SPU に更新 OK の通知
　　以下 2. から 5. の繰り返し

　　1. から 5. までの各処理の詳細を記述します。
　　1. では cellGcmInlineTransfer() にて uint32_t * 4 のデータ(0xbeefbeef, 0xbeefbeef, 0xbeefbeef, 0xbeefbeef)
    を転送します。

　　2. では SPU は duck_spu.cpp 内の以下のコードで RSX(R) からの通知を待ちます。
　　受け取った後はクリアを行い、書き込みを実行しています。

        // wait notify from RSX
        while (1) {
            mfcGetData(read_label_buffer, spuparam.spu_read_label_addr, 16);
            if (read_label_buffer[0] == 0xbeefbeef) {
                for ( int i = 0; i < 4; i++)
                    read_label_buffer[i] = 0xcafecafe;
                // clear the buffer
                mfcPutData(read_label_buffer, spuparam.spu_read_label_addr, 16);
                break;
            }
        }
        mfcPutData(dma_buffer, buff_ea, size);

　　3. では RSX(R) に対して更新終了を通知するために以下のようなデータを作成し、通知を行います。

        write_label_buffer[0] = 0xabcdabcd;
        write_label_buffer[1] = 0xabcdabcd;
        write_label_buffer[2] = 0xabcdabcd;
        write_label_buffer[3] = 0xabcdabcd;
        mfcPutData(write_label_buffer, spuparam.spu_write_label_addr, 16);

    通知後は 2. と同様に期待した値になるまで待ちます。

    4. RSX(R) は 
	cellGcmSetWaitLabel(64, 0xabcdabcd) 
　　で指定したラベル領域が期待した値に
　　変わるのを待つことで SPU のパラメータ更新を確認します。通知を受け取った後、
	setShaderProgram()
    にてフラグメントプログラムの設定、そして描画を行います。

　　5. 描画終了後、
	cellGcmSetWriteCommandLabel(64, 0xdddddddd)
    として SPU との同期領域をクリアし、続いて 1. と同様に 2. で SPU が待っている
　　値を通知し、次の描画へと進む仕組みになっています。


<ファイル>
    Makefile      : Makefile
    disp.cpp      : ディスプレイ関連の設定用ファイル
    disp.h        : ディスプレイ関連の設定用ヘッダファイル
    fs.cpp        : ファイルシステム操作用ファイル
    fs.h          : ファイルシステム操作用ヘッダファイル
    geometry.cpp  : オブジェクト描画用ファイル
    geometry.h    : オブジェクト描画用ヘッダファイル
    gtf.cpp       : GTF テクスチャ読み込み用ファイル
    gtf.h         : GTF テクスチャ読み込み用ヘッダファイル
    memory.cpp    : ローカルメモリ、メインメモリ操作用ファイル
    memory.h      : ローカルメモリ、メインメモリ操作用ヘッダファイル
    pad.cpp       : コントローラ操作用ファイル
    pad.h         : コントローラ操作用ヘッダファイル
    duck.cpp      : メインプログラム
    readme_e.txt  : readme テキストファイル（英語）
    readme_j.txt  : readme テキストファイル（日本語）
    shader.cpp    : シェーダプログラム設定用ファイル
    shader.h      : シェーダプログラム設定用ヘッダファイル
    texture.cpp   : テクスチャ設定用ファイル
    texture.h     : テクスチャ設定用ヘッダファイル
    fpshader.cg   : フラグメントシェーダプログラム
    vpshader.cg   : バーテクスシェーダプログラム
    images/duck256.gtf : 描画オブジェクト用テクスチャ
    data/duck.smesh    : 描画オブジェクト用ジオメトリデータ

<操作方法>
    ・描画オブジェクトの描画モード切替
        △　   : 描画モードをポリゴンかラインに交互に切り替える

    ・描画オブジェクトの増加、減少
        R1     : 描画オブジェクトを1つ増やす
        R2     : 描画オブジェクトを 1 つ減らす

<注意事項>
    本サンプルでは sce-cgcdisasm で出力したヘッダ情報をメインプログラム
　　にて使用するために作成されているため、本来ならば不要な処理も含まれて
　　おり、ハードウェアの機能を最適に使用するような記述になっておりません
　　のでアプリケーションに組み込む際にはご注意ください。

