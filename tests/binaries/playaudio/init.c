/* SCE CONFIDENTIAL
 PlayStation(R)3 Programmer Tool Runtime Library 475.001
 * Copyright (C) 2009 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

//J  このサンプルではシステムの出力チャンネル数設定を、システム設定と
//J  アプリケーションの使用チャンネル数設定に基づいて設定します。
//J  従って、アプリケーションは使用するチャンネル数を設定する必要があります。
//J  アプリケーションの使用チャンネル数を AUDIO_CHANNELS に設定してください。
//J  このサンプルでは、.vcproj ファイル内、あるいは Makefile 内で定義されています。
//E  In this sample, the system setting for the number of output channels 
//E  is determined by both the system setting and the application setting 
//E  regarding the number of channels to use. Thus the number of channels 
//E  to use must be specified in the application. Set this number to 
//E  AUDIO_CHANNELS.
//E  In this sample, the settings are defined in either the .vcproj file or 
//E  the Makefile.

#define CONTENTS_CH_NUNBER_IS_6CH  6
#define CONTENTS_CH_NUNBER_IS_8CH  8
#define CONTENTS_CH_NUNBER_IS_2CH  2

#ifndef AUDIO_CHANNELS
#define AUDIO_CHANNELS  CONTENTS_CH_NUNBER_IS_6CH
#endif

//J  コンテンツは、HDMIや光出力でのコピーコントロール設定を決める必要があります。
//J  そのための定義です。
//J  アプリケーション内から、第三者の著作権付きコンテンツが出力される可能性がある場合は
//J  CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER
//J  に設定する必要があります。
//J  そうでない場合は、CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE でよいでしょう。
//E  The application must specify the copy control/protection settings for 
//E  HDMI and digital out (optical). This definition is used for that 
//E  purpose.
//E  If it is possible for third-party copyrighted content to be output 
//E  from within the application, CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER 
//E  must be specified. Otherwise, CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE 
//E  is sufficient.

#define SET_COPYBIT			CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE
//#define SET_COPYBIT		CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER


//J  このファイル内の libpad 初期化/設定/読み込みルーチンを使用する指定です。
//J  アプリケーション内で libpad の初期化/設定/読み込みを直接行っている場合は、
//J  0 を設定して下さい。
//J  このサンプルでは、.vcproj ファイル内、あるいは Makefile 内で定義されています。
//E  This specifies the use of the libpad initialization/configuration/load
//E  routine in the file. If libpad is initialized/configured/loaded 
//E  directly in the application, specify 0.
//E  In this sample, this setting is defined in either the .vcproj file or 
//E  the Makefile.

#ifndef USE_LIBPAD_IN_INIT_C
#define USE_LIBPAD_IN_INIT_C	1
#endif

#include <stdio.h>
#include <stdlib.h>				/* memalign() */
#include <string.h>				/* memset() */
#include <sys/timer.h>			/* sys_timer_usleep() */
#include <sys/return_code.h>	/* CELL_OK */

#include <sysutil/sysutil_sysparam.h> /* cellVideoOut*()/cellAudioOut*() */

#include <cell/gcm.h>			/* cellGcm*() */
#if USE_LIBPAD_IN_INIT_C != 0
#include <cell/pad.h>			/* cellPad*() */
#endif /* USE_LIBPAD_IN_INIT_C != 0 */

#define COLOR_DEPTH				4	/* ARGB8 */

/* Video/Audio Out: check interval is 5msec and give up after 10sec */
#define AVOUT_CHECK_WAIT		5000 /* 5msec */
#define AVOUT_CHECK_TIMEOUT		(10 * ((1000 * 1000) / AVOUT_CHECK_WAIT)) /* 10sec */

/* called by main routine */
int  systemUtilityInit(void *);
void systemUtilityQuit(void);

static int  systemUtilityAudioInit(void);
static void systemUtilityAudioQuit(void);
#if 0
static int systemUtilityAudioInit__ReferenceToolOnly__NotForTitle__AnalogAudioOutMax8ch(void);
#endif

static int  systemUtilityVideoInit(void);
static void systemUtilityVideoQuit(void);

static int  libGcmInit(void);
static void libGcmQuit(void);

#if USE_LIBPAD_IN_INIT_C != 0
static int  libPadInit(void);
static void libPadQuit(void);
#endif

void v_interval(void);
static void checkCallback(void);
#if USE_LIBPAD_IN_INIT_C != 0
static void padRead(void);
#endif
static void videoFlip(void);

#define __KB__					1024
#define __MB__					(__KB__ * __KB__)

#define CALLBACK_SLOT			0
#define MAX_PAD					1

#define COLOR_BUFFER_NUM		2	/* double buffering */
#define IO_BUFFER_SIZE			( 1 * __MB__)
#define COMMAND_BUFFER_SIZE		(64 * __KB__)

static uint32_t			g_heap = 0;
static uint32_t			g_color_pitch = 0;
static uint32_t			g_width = 0;
static uint32_t			g_height = 0;
static uint32_t			g_color_offset [COLOR_BUFFER_NUM];
static CellGcmSurface	g_surface;
static float            g_viewport_scale [4];
static float            g_viewport_offset [4];

/* ----------------------------------------------------------------
 *
 *  S Y S T E M   U T I L I T Y
 *
 * ---------------------------------------------------------------- */

int systemUtilityInit(void *callback)
{
	int ret;

	/*
	 * Registering sysutil callback is done first:
	 *     in video/audio initialization part, sysutil callback must
	 *     be checked, for PS button handling.
	 */

	/* system utility callback */
	if (callback != NULL){
		ret = cellSysutilRegisterCallback(CALLBACK_SLOT, (CellSysutilCallback)callback, NULL);
		if (ret < 0){
			printf("cellSysutilRegisterCallback() failed (0x%x)\n", ret);
			return -99;
		}
	}

	ret = systemUtilityVideoInit();
	if (ret < 0){
		printf("systemUtilityVideoInit() failed (0x%x)\n", ret);
		return -1;
	}

	ret = libGcmInit();
	if (ret < 0){
		printf("libGcmInit() failed (0x%x)\n", ret);
		return -2;
	}

	ret = systemUtilityAudioInit();
	if (ret < 0){
		printf("systemUtilityAudioInit() failed (0x%x)\n", ret);
		return -3;
	}

#if USE_LIBPAD_IN_INIT_C != 0
	ret = libPadInit();
	if (ret < 0){
		printf("libPadInit() failed (0x%x)\n", ret);
		return -4;
	}
#endif /* USE_LIBPAD_IN_INIT_C != 0 */

	return CELL_OK;
}

void systemUtilityQuit(void)
{
	int ret;

#if USE_LIBPAD_IN_INIT_C != 0
	libPadQuit();
#endif
	systemUtilityAudioQuit();
	libGcmQuit();
	systemUtilityVideoQuit();

	ret = cellSysutilUnregisterCallback(CALLBACK_SLOT);
	if (ret < 0){
		printf("cellSysutilUnregisterCallback() failed (0x%x)\n", ret);
	}

	return;
}

static void checkCallback(void)
{
	int ret;

	ret = cellSysutilCheckCallback();
	if (ret != CELL_OK){
		printf("cellSysutilCheckCallback() failed (0x%x)\n", ret);
	}

	return;
}

/* ----------------------------------------------------------------
 *
 *  A U D I O
 *
 * ---------------------------------------------------------------- */
//J  (libaudioでは無く)システムのオーディオ出力設定を行う関数です。
//E  This function configures the audio output settings of the system (not 
//E  libaudio).

//J アプリケーションコードからのオーディオ出力設定は必須ではありません。
//J 多くの設定はPARAM.SFOで可能です。
//J しかし、アプリケーションコードからのオーディオ出力設定を行うことは出来ます。
//J 以下にその場合のコード例を提示します。
//E It is not necessarily the case that audio output must be set from 
//E application code.
//E Many of the settings can be configured with PARAM.SFO.
//E And audio output can be set from application code as well.
//E See the following code examples as below:

//J  なお、サウンドエンジンとして MultiStream を利用する場合は、
//J  サウンド設定を、このサンプルの方式ではなく、MultiStream で用意された
//J  関数で行うことも出来ます。詳しくは cellMSSystemConfigureSysUtilEx() を
//J  ご参照下さい。
//E  If you use MultiStream as a sound engine, you can configure the
//E  sound settings not only with the way of this sample but also with
//E  the functions MultiStream provides.
//E  As for details, please see cellMSSystemConfigureSysUtilEx().

static int systemUtilityAudioInit(void)
{
	int ret;
	int count;
	int ch_pcm;
	int ch_ac3;
	int ch_dts;
	CellAudioOutConfiguration a_config;
	CellAudioOutState a_state;

	//   ---------------------------------------------------------
	//J 判断の基準は、出力がHDMIかアナログか?ではありません。
	//J 「どのフォーマットだと何chだせるのか」を参照し、判断します。
	//J  具体的には、
	//J フォーマットごとの使えるチャンネル数を取得してから
	//J オーディオ出力設定、内蔵ダウンミキサ設定を行います。
	//E Judgment is made by reference to "how many channels is 
	//E available for each format", not depending upon if sound
	//E is output by HDMI or by analogue method.
	//E In concrete, this obtains how many channels are available
	//E for each format and then sets audio output and internal
	//E downmixer.
	//  ---------------------------------------------------------

	//J リニアPCMに限った場合の使えるチャンネル数を調べます。
	//J     なお、リニアPCMが利用出来ないケースはありません。
	//J     少なくとも2chは利用可能です。
	//E Checks out how many channels are available for a case
	//E only linear PCM is used.
	//E      There is no case linear PCM can not be used.
	//E      At least, the 2 channel is available.
	ch_pcm = cellAudioOutGetSoundAvailability(CELL_AUDIO_OUT_PRIMARY,
											  CELL_AUDIO_OUT_CODING_TYPE_LPCM,
											  CELL_AUDIO_OUT_FS_48KHZ, 0);

	//J システムのドルビーエンコーダを使おうとした場合の、使えるチャンネル数を調べます。
	//J 利用出来ない場合は 数として0が返ります。
	//J (注1)
	//E Checks out how many channels are available for a case 
	//E Dolby(TM) encoder of the system is used.
	//E  If this can not be used, 0 is returned.
	//E (Note 1)
	ch_ac3 = cellAudioOutGetSoundAvailability(CELL_AUDIO_OUT_PRIMARY,
											  CELL_AUDIO_OUT_CODING_TYPE_AC3,
											  CELL_AUDIO_OUT_FS_48KHZ, 0);

	//J システムのDTSエンコーダを使おうとした場合の、使えるチャンネル数を調べます。
	//J 利用出来ない場合は 数として0が返ります。
	//J (注1)
	//E Checks out how many channels are available for a case 
	//E DTS encoder of the system is used.
	//E If this can not be used, 0 is returned.
	//E (Note 1)
	ch_dts = cellAudioOutGetSoundAvailability(CELL_AUDIO_OUT_PRIMARY,
											  CELL_AUDIO_OUT_CODING_TYPE_DTS,
											  CELL_AUDIO_OUT_FS_48KHZ, 0);

	//-----------------------------------------------------
	//J 以下から判断ですが、アプリケーションごとに判断基準は異なると思われます。
	//J (例えば 遅延を重要視するならチャンネル数が少なくてもリニアPCMを優先するとか)
	//E Judgment is made by the followings, and the criterion 
	//E for the judgment may vary depending upon applications.
	//E (For example, if it emphasizes delaying, linear PCM is 
	//E prioritized even with small number of the channels.)

	//J  ここではいくつかの例を#defineで分離して提示します。
	//E  A few examples are shown here (each set off by #define).

	//J 設定用の構造体をzero-clearしておきます。
	//E The target structure must be filled with zero
	(void)memset(&a_config, 0, sizeof(CellAudioOutConfiguration));

#if AUDIO_CHANNELS == CONTENTS_CH_NUNBER_IS_6CH
	//J  --- 5.1chコンテンツの場合  -----------------------------
	//E  --- Content created for 5.1ch ----------------
	if (ch_pcm >= 6){
		//J リニアPCMの6chが利用出来るなら、それを採用
		//E If 6ch of linear PCM is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J 内蔵ダウンミキサは未使用
		//E Internal downmixer is not used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
	else if (ch_ac3 >= 6){
		//J else if ドルビーが利用出来るなら、それを採用
		//E else if Dolby(TM) is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_AC3;
		//J 内蔵ダウンミキサは未使用
		//E Internal downmixer is not used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
	else if (ch_dts >= 6){
		//J else if DTSが利用出来るなら、それを採用
		//E else if DTS is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_DTS;
		//J 内蔵ダウンミキサは未使用
		//E Internal downmixer is not used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
	else {
		//J else  どれも利用できないならリニアPCMの2chで運用
		//E else if neither is available, use 2ch of linear PCM.
		a_config.channel   = 2;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J アプリケーションとしてはスペシャルなダウンミックス等は特に行わないので、
		//J 5.1ch->2.0chの内蔵ダウンミキサ(TYPE_A)を利用 
		//E On applications, special downmixing etc. are not performed
		//E and internal downmixer (5.1ch->2.0ch: TYPE_A) is used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_TYPE_A;
	}
	//J エンコーダ採用の優先順位はお好みでどうぞ
	//E  The priority of the encoders can be specified as convenient.
#elif AUDIO_CHANNELS == CONTENTS_CH_NUNBER_IS_8CH
	//J  --- 7.1chで制作されたコンテンツの場合  ----------------
	//J 基本は5.1chの場合と同じですが、ダウンミキサの扱いが異なります。
	//E  --- Content created for 7.1ch ----------------
	//E  In general, this is handled the same as 5.1ch content, except for 
	//E  the downmixing.

	if (ch_pcm >= 8){
		//J リニアPCMの8chが利用出来るなら、それを採用
		//E If 8ch of linear PCM is available, this is adopted.
		a_config.channel   = 8;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J 内蔵ダウンミキサは未使用
		//E Internal downmixer is not used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
	else if (ch_pcm >= 6){
	
		//J リニアPCMの6chが利用出来るなら、それを採用
		//E If 6ch of linear PCM is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J 7.1ch->5.1chの内蔵ダウンミキサ(TYPE_B)を利用 
		//E Internal downmixer (7.1ch->5.1ch: TYPE_B) is used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_TYPE_B;
	}
	else if (ch_dts >= 6){
		//J else if DTSが利用出来るなら、それを採用
		//E else if DTS is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_DTS;
		//J 7.1ch->5.1chの内蔵ダウンミキサ(TYPE_B)を利用 
		//E Internal downmixer (7.1ch->5.1ch: TYPE_B) is used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_TYPE_B;
	}
	else if (ch_ac3 >= 6){
		//J else if ドルビーが利用出来るなら、それを採用
		//E else if Dolby(TM) is available, this is adopted.
		a_config.channel   = 6;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_AC3;
		//J 7.1ch->5.1chの内蔵ダウンミキサ(TYPE_B)を利用 
		//E Internal downmixer (7.1ch->5.1ch: TYPE_B) is used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_TYPE_B;
	}
	else {
		//J else  どれも利用できないならリニアPCMの2chで運用
		//E else if neither is available, use 2ch of linear PCM.
		a_config.channel   = 2;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J アプリケーションとしてはスペシャルなダウンミックス等は特に行わないので、
		//J 7.1ch->2.0chの内蔵ダウンミキサ(TYPE_A)を利用 
		//E On applications, special downmixing etc. are not performed
		//E and internal downmixer (7.1ch->2.0ch: TYPE_A) is used.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_TYPE_A;
	}
	//J エンコーダ採用の優先順位はお好みでどうぞ
	//E  The priority of the encoders can be specified as convenient.
#else /* AUDIO_CHANNELS */
	{
		//J  コンパイラ警告抑制のためのみ
		//E  just for avoiding compiler warning
		(void)ch_pcm; (void)ch_ac3; (void)ch_dts;
		//J  ---- 2ch で出力する/したい -----------------------------------
		//J  コンテンツが2chのみで制作されている場合と
		//J  再生系の遅延を最小にするためにあえて2chを選択する場合、があるでしょう。
		//E  ---- Output in 2ch -----------------------------------
		//E  This can be necessary if content was created for 2ch, or 
		//E  if 2ch output is selected for the purpose of minimizing 
		//E  delays in playback.
		a_config.channel   = 2;
		a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		//J ソースが2chならダウンミキサは不要なはずです。
		//E  If the source is 2ch, downmixing is unnecessary.
		a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
#endif /* AUDIO_CHANNELS */

	//J 決めた内容をシステムへ設定します。 通常、一回で成功します。
	//E The fixed contents is configured into the system. 
	//E Normally, this succeeds on the first attempt.
	for (count = 0; count < AVOUT_CHECK_TIMEOUT; count ++){
		ret = cellAudioOutConfigure(CELL_AUDIO_OUT_PRIMARY, &a_config, NULL, 0);
		if (ret == CELL_OK){
			break;
		}
		else if (ret != (int)CELL_AUDIO_IN_ERROR_CONDITION_BUSY){
			break;
		}
		sys_timer_usleep(AVOUT_CHECK_WAIT);

		//J  このループでトラブルが発生した場合でもPSボタンが効くように
		//E  Use checkCallback(); so the PS button will work even when a 
		//E  problem occurs in this loop.
		checkCallback();		/* for PS button  */
	}
	if (count >= AVOUT_CHECK_TIMEOUT){
		printf("cellAudioOutConfigure(): audio output could NOT be configured; give up.\n");
	}
	if (ret != CELL_OK){
		printf("cellAudioOutConfigure() failed (0x%x)\n", ret);
		return ret;
	}

	//J 本当にシステムが動作を始めるまで待ちます。
	//E Waits until the system works actually.
	for (count = 0; count < AVOUT_CHECK_TIMEOUT; count ++){
		ret = cellAudioOutGetState(CELL_AUDIO_OUT_PRIMARY, 0, &a_state);
		if (ret == CELL_OK){
			if (a_state.state == CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED){
				//J muteが解除されたので抜けます。
				//E Since mute is cancelled, the processing exits.
				break;
			}
		}
		else if (ret != (int)CELL_AUDIO_OUT_ERROR_CONDITION_BUSY){
			//J 「システムソフトウェアが状態を変更中」以外。
			//J 想定外のエラー。リトライやめて抜けます。
			//E Except for "System software is currently changing the state".
			//E Unexpected error. Cancels retrying and exits.
			break;
		}
		sys_timer_usleep(AVOUT_CHECK_WAIT);

		//J  このループでトラブルが発生した場合でもPSボタンが効くように
		//E  Use checkCallback(); so the PS button will work even when a 
		//E  problem occurs in this loop.
		checkCallback();		/* for PS button  */
	}
	if (count >= AVOUT_CHECK_TIMEOUT){
		printf("cellAudioOutGetState(): audio output status is NOT enabled; give up.\n");
	}
	if (ret != CELL_OK){
		printf("cellAudioOutGetState() failed (0x%x)\n", ret);
		return ret;
	}

	/* For HDMI/SPDIF: copy generation management */
	ret = cellAudioOutSetCopyControl(CELL_AUDIO_OUT_PRIMARY, SET_COPYBIT);
	if (ret != CELL_OK){
		printf("cellAudioOutSetCopyControl() failed (0x%x)\n", ret);
		return ret;
	}

	printf("[Audio environment: %d-ch]\n", a_config.channel);
	if (a_config.encoder != CELL_AUDIO_OUT_CODING_TYPE_LPCM){
		printf("[Audio bitstream encoder is specified: %s]\n",
			   ( a_config.encoder == CELL_AUDIO_OUT_CODING_TYPE_AC3) ? "AC3" :
			   ((a_config.encoder == CELL_AUDIO_OUT_CODING_TYPE_DTS) ? "DTS" : "???"));
	}

	return CELL_OK;				/* or, return (ch_pcm) */
}
//J (注1)
//J SPDIFの場合、PS3は、利用するステレオ機器がDolbyやDTSをdecode可能かの情報を自動では取得出来ません。
//J これらの情報はユーザーがPS3に設定する必要があります。
//J これを設定するのはXMB内の「サウンド設定→音声出力設定」における チェックボックスの指定 でです。
//J 指定は厳密には「利用指定」ではなく、「利用許可」ですが、使いたいデコーダではない方の
//J 指定をはずすことで、デコーダの指定も可能になります。
//J (ユーザーがデコーダの選択をしたい場合は、ここで吸収します)
//J cellAudioOutGetSoundAvailability()の結果は、このチェックボックスの指定状態を反映します。
//E (Note 1)
//E As for SPDIF, PS3(TM) can not obtain information if a stereo device
//E to use can decode Dolby(TM) and DTS. This information must be set onto
//E PS3(TM), on the user side. You can set it by ticking the checkbox in the
//E "Sound Settings" -> "Audio Output Settings" in XMB(TM). Strictly speaking,
//E this is not "specification of the usage" but "permission for the usage".
//E You can specify the decoder to use by unchecking the checkbox of the decoder
//E which you don't use. (If the decoder is selected, this is set here.)
//E The result of cellAudioOutGetSoundAvailability() reflects how the checkboxes
//E are specified.

#if 0
/*
 * For using analog 8ch output of Reference Tools only
 */
//J		この関数はDEH-1000においてアナログ8ch出力を利用する場合専用のものです。
//J		systemUtilityAudioInit()を置き換えて使って下さい。
//J		当然、最終の商品で使ってはいけません。
//E     This function is used only for analog 8ch output on DEH-1000.
//E     Replace systemUtilityAudioInit() with this function.
//E     It cannot be used in the final product.

static int systemUtilityAudioInit__ReferenceToolOnly__NotForTitle__AnalogAudioOutMax8ch(void)
{
	int ret;
	int count;
	CellAudioOutConfiguration a_config;
	CellAudioOutState a_state;

	(void)memset(&a_config, 0, sizeof(CellAudioOutConfiguration));

	/*
	 * not using `cellAudioOutGetSoundAvailability()' ... FOR TEST ONLY
	 */

	//J リニアPCMの8chを強制採用
	a_config.channel   = 8;						/* all channels are out via analog 8ch output */
	a_config.encoder   = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	//J 内蔵ダウンミキサは未使用
	//E Internal downmixer is not used.
	a_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;

	//J 決めた内容をシステムへ設定します。 通常、一回で成功します。
	//E The fixed contents is configured into the system. 
	//E Normally, this succeeds on the first attempt.
	for (count = 0; count < AVOUT_CHECK_TIMEOUT; count ++){
		ret = cellAudioOutConfigure(CELL_AUDIO_OUT_PRIMARY, &a_config, NULL, 0);
		if (ret == CELL_OK){
			break;
		}
		else if (ret != (int)CELL_AUDIO_IN_ERROR_CONDITION_BUSY){
			break;
		}
		sys_timer_usleep(AVOUT_CHECK_WAIT);
		checkCallback();		/* for PS button  */
	}
	if (count >= AVOUT_CHECK_TIMEOUT){
		printf("cellAudioOutConfigure(): audio output could NOT be configured; give up.\n");
	}
	if (ret != CELL_OK){
		printf("cellAudioOutConfigure() failed (0x%x)\n", ret);
		return ret;
	}

	//J 本当にシステムが動作を始めるまで待ちます。
	//E Waits until the system works actually.
	for (count = 0; count < AVOUT_CHECK_TIMEOUT; count ++){
		ret = cellAudioOutGetState(CELL_AUDIO_OUT_PRIMARY, 0, &a_state);
		if (ret == CELL_OK){
			if (a_state.state == CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED){
				//J muteが解除されたので抜けます。
				//E Since mute is cancelled, the processing exits.
				break;
			}
		}
		else if (ret != (int)CELL_AUDIO_OUT_ERROR_CONDITION_BUSY){
			//J 「システムソフトウェアが状態を変更中」以外。
			//J 想定外のエラー。リトライやめて抜けます。
			//E Except for "System software is currently changing the state".
			//E Unexpected error. Cancels retrying and exits.
			break;
		}
		sys_timer_usleep(AVOUT_CHECK_WAIT);
		checkCallback();		/* for PS button  */
	}
	if (count >= AVOUT_CHECK_TIMEOUT){
		printf("cellAudioOutGetState(): audio output status is NOT enabled; give up.\n");
	}
	if (ret != CELL_OK){
		printf("cellAudioOutGetState() failed (0x%x)\n", ret);
		return ret;
	}

	/* For HDMI/SPDIF: copy generation management */
	ret = cellAudioOutSetCopyControl(CELL_AUDIO_OUT_PRIMARY, SET_COPYBIT);
	if (ret != CELL_OK){
		printf("cellAudioOutSetCopyControl() failed (0x%x)\n", ret);
		return ret;
	}

	printf("[Audio environment: %d-ch]\n", a_config.channel);

	return CELL_OK;				/* or, return (channels) */
}
#endif /* 0 */

static void systemUtilityAudioQuit(void)
{
	/* do nothing */
}

/* ----------------------------------------------------------------
 *
 *  V I D E O / G R A P H I C S
 *
 * ---------------------------------------------------------------- */

static int systemUtilityVideoInit(void)
{
	int ret;
	int count;
	CellVideoOutState			v_state;
	CellVideoOutResolution		v_resolution;
	CellVideoOutConfiguration	v_config;

	(void)memset(&v_config, 0, sizeof(CellVideoOutConfiguration));

	/* video configuration */
	ret = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &v_state);
	if (ret != CELL_OK){
		printf("cellVideoOutGetState() failed (0x%x)\n", ret);
		return ret;
	}
	ret = cellVideoOutGetResolution(v_state.displayMode.resolutionId, &v_resolution);
	if (ret != CELL_OK){
		printf("cellVideoOutGetResolution() failed (0x%x)\n", ret);
		return ret;
	}

	v_config.resolutionId = v_state.displayMode.resolutionId;
	v_config.format       = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	v_config.pitch        = v_resolution.width * COLOR_DEPTH;
	ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &v_config, NULL, 0);
	if (ret != CELL_OK){
		printf("cellVideoOutConfigure() failed (0x%x)\n", ret);
		return ret;
	}

	/* check whether video configuration is finished */
	for (count = 0; count < AVOUT_CHECK_TIMEOUT; count ++){
		ret = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &v_state);
		if (ret == CELL_OK){
			if (v_state.state == CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED){
				//J muteが解除されたので抜けます。
				//E Since mute is cancelled, the processing exits.
				break;
			}
		}
		else if (ret != (int)CELL_VIDEO_OUT_ERROR_CONDITION_BUSY){
			//J 「システムソフトウェアが状態を変更中」以外。
			//J 想定外のエラー。リトライやめて抜けます。
			//E Except for "System software is currently changing the state".
			//E Unexpected error. Cancels retrying and exits.
			break;
		}
		sys_timer_usleep(AVOUT_CHECK_WAIT);
		checkCallback();		/* for PS button  */
	}
	if (count >= AVOUT_CHECK_TIMEOUT){
		printf("cellVideoOutGetState(): video output status is NOT enabled; give up.\n");
	}
	if (ret != CELL_OK){
		printf("cellVideoOutGetState() failed (0x%x)\n", ret);
		return ret;
	}

	g_color_pitch = v_config.pitch;
	g_width       = v_resolution.width;
	g_height      = v_resolution.height;

	return CELL_OK;
}

static void systemUtilityVideoQuit(void)
{
	/* do nothing */
}

static void *getAddr(uint32_t *h, const uint32_t align, const uint32_t size)
{
	uint32_t s = (size + (__KB__ - 1)) & (~(__KB__ - 1)); /* 1 KB alignment */
	uint32_t old;

	*h = (*h + (align - 1)) & (~(align - 1)); /* specified alignment */
	old = *h;
	*h += s;
	return (void *)old;
}

static int libGcmInit(void)
{
	int ret;
	int i;
	void		   *addr, *laddr, *p;
	CellGcmConfig	config;
	uint32_t		color_size = 0;

	addr = memalign(__MB__, IO_BUFFER_SIZE);
	ret = cellGcmInit(COMMAND_BUFFER_SIZE, IO_BUFFER_SIZE, addr);
	if (ret != CELL_OK){
		printf("cellGcmInit() failed (0x%x)\n", ret);
		return ret;
	}

	color_size = g_width * g_height * COLOR_DEPTH;

	cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

	cellGcmGetConfiguration(&config);
	g_heap = (uint32_t)config.localAddress;

	/* register surface for display */
	laddr = getAddr(&g_heap, 16, COLOR_BUFFER_NUM * color_size);
	for (i = 0; i < COLOR_BUFFER_NUM; i ++){
		p = (void *)((uint32_t)laddr + (i * color_size));
		ret = cellGcmAddressToOffset(p, &g_color_offset [i]);
		if (ret != CELL_OK){
			printf("cellGcmAddressToOffset() failed (0x%x)\n", ret);
			return ret;
		}
		ret = cellGcmSetDisplayBuffer(i, g_color_offset [i],
									  g_color_pitch,
									  g_width, g_height);
		if (ret != CELL_OK){
			printf("cellGcmSetDisplayBuffer() failed (0x%x)\n", ret);
			return ret;
		}
	}

	/* setup surface */
	g_surface.colorFormat 		= CELL_GCM_SURFACE_A8R8G8B8;
	g_surface.colorTarget		= CELL_GCM_SURFACE_TARGET_0;
	g_surface.colorLocation [0]	= CELL_GCM_LOCATION_LOCAL;
	g_surface.colorLocation [1]	= CELL_GCM_LOCATION_LOCAL;
	g_surface.colorLocation [2]	= CELL_GCM_LOCATION_LOCAL;
	g_surface.colorLocation [3]	= CELL_GCM_LOCATION_LOCAL;
	g_surface.colorOffset   [0]	= 0;
	g_surface.colorOffset   [1] = 0;
	g_surface.colorOffset   [2] = 0;
	g_surface.colorOffset   [3] = 0;
	g_surface.colorPitch    [0]	= g_color_pitch,
	g_surface.colorPitch    [1]	= 64;
	g_surface.colorPitch    [2]	= 64;
	g_surface.colorPitch    [3]	= 64;

	g_surface.depthFormat 		= CELL_GCM_SURFACE_Z16;
	g_surface.depthLocation		= CELL_GCM_LOCATION_LOCAL;
	g_surface.depthOffset		= 0;
	g_surface.depthPitch		= 64;

	g_surface.type				= CELL_GCM_SURFACE_PITCH;
	g_surface.antialias			= CELL_GCM_SURFACE_CENTER_1;

	g_surface.width 			= g_width;
	g_surface.height 			= g_height;
	g_surface.x 				= 0;
	g_surface.y 				= 0;

	g_viewport_scale  [0] = g_width  *   0.5f;
	g_viewport_scale  [1] = g_height * (-0.5f); /* -0.5f ... left-bottom is the origin */
	g_viewport_scale  [2] = 0.5f;				/* (max - min) * 0.5f ... max = 1.0f, min = 0.0f */
	g_viewport_scale  [3] = 0.0f;				/* always 0.0f */
	g_viewport_offset [0] = g_width  * 0.5f;	/* x + w * 0.5f ... x = 0 */
	g_viewport_offset [1] = g_height * 0.5f;	/* y + h * 0.5f ... y = 0 */
	g_viewport_offset [2] = 0.5f;				/* (max + min) * 0.5f ... max = 1.0f, min = 0.0f */
	g_viewport_offset [3] = 0.0f;				/* always 0.0f */

	return CELL_OK;
}

static void libGcmQuit(void)
{
	cellGcmFinish(gCellGcmCurrentContext, 0);
}

static void videoFlip(void)		/* just a display clearing */
{
	int ret;
	static int first = 1;
	static uint32_t frame_index = 0;
	static int color = 0;
	static int delta = 1;

	/* change display offset */
	g_surface.colorOffset [0] = g_color_offset [frame_index];
	cellGcmSetSurface(gCellGcmCurrentContext, &g_surface);

	/* no depth buffer */
	cellGcmSetDepthTestEnable(gCellGcmCurrentContext, CELL_GCM_FALSE);

	/* set viewport */
	cellGcmSetViewport(gCellGcmCurrentContext,
					   0, 0,		/* x, y */
					   g_width, g_height,
					   0.0f, 1.0f,	/* min, max */
					   g_viewport_scale, g_viewport_offset);

	/* clear surface */
	cellGcmSetClearColor(gCellGcmCurrentContext, ((color <<  0) |
												  (color <<  8) |
												  (color << 16) |
												  (color << 24)));
	cellGcmSetClearSurface(gCellGcmCurrentContext, (CELL_GCM_CLEAR_R |
													CELL_GCM_CLEAR_G |
													CELL_GCM_CLEAR_B |
													CELL_GCM_CLEAR_A));
	if (color > 100) delta = -1;
	if (color <= 0)  delta =  1;
	color = color + delta;

	/* wait until the previous flip executed */
	if (first != 1){
		while (cellGcmGetFlipStatus() != 0){
			sys_timer_usleep(300);
		}
	}
	cellGcmResetFlipStatus();

	/* flip */
	ret = cellGcmSetFlip(gCellGcmCurrentContext, frame_index);
	if (ret != CELL_OK) return;
	cellGcmFlush(gCellGcmCurrentContext);

	cellGcmSetWaitFlip(gCellGcmCurrentContext);

	/* New render target */
	frame_index = (frame_index + 1) % COLOR_BUFFER_NUM;

	first = 0;
}

/* ----------------------------------------------------------------
 *
 *  U S B   H I D   /   C O N T R O L L E R
 *
 * ---------------------------------------------------------------- */

/*
 *  USE_LIBPAD_IN_INIT_C
 *    != 0, when controller handling code is used in init.c.
 */

#if USE_LIBPAD_IN_INIT_C != 0
static int libPadInit(void)
{
	int ret;

	/* Controller configuration */
	ret = cellPadInit(MAX_PAD);
	if (ret != CELL_OK){
		printf("cellPadInit() failed (0x%x)\n", ret);
		return ret;
	}

	return CELL_OK;
}

static void libPadQuit(void)
{
	int ret;

	/* Controller configuration */
	ret = cellPadEnd();
	if (ret != CELL_OK){
		printf("cellPadEnd() failed (0x%x)\n", ret);
	}

	return;
}

/* just controller reading */
static void padRead(void)
{
	int i;
	int ret;
	CellPadInfo2 pad_info;
	CellPadData pad_data;

	ret = cellPadGetInfo2(&pad_info);
	if (ret != CELL_PAD_OK){
		printf("cellPadGetInfo2() failed (0x%x)\n", ret);
		return;
	}
	for (i = 0; i < MAX_PAD; i ++){
		if ((pad_info.port_status [i] & CELL_PAD_STATUS_CONNECTED) == 0){
			/* disconnected */
			continue;
		}
		ret = cellPadGetData(i, &pad_data);
		if (ret != CELL_PAD_OK ||
			pad_data.len == 0){
			continue;
		}
	}

	return;
}
#endif /* USE_LIBPAD_IN_INIT_C != 0 */

/* ----------------------------------------------------------------
 *
 *  I N T E R V A L   F U N C T I O N
 *
 * ---------------------------------------------------------------- */

void v_interval (void)
{
	checkCallback();
#if USE_LIBPAD_IN_INIT_C != 0
	padRead();
#endif
	videoFlip();
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * End:
 * vi:ts=4:sw=4
 */
