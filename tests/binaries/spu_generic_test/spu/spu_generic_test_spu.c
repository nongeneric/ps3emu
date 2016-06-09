/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <stdint.h>
#include <cell/sync.h>
#include <cell/dma.h>
#include <cellstatus.h>
#include <string.h>
#include <stdio.h>

#include <spu_internals.h>
#include <spu_intrinsics.h>

uint32_t count_array[4] __attribute__ ((aligned (128)));
char message_buffer[128] __attribute__ ((aligned (128)));
uint32_t tag = 3;
uint64_t u64buffer[8] __attribute__ ((aligned (128)));
vector unsigned int outputsBuffer[386] __attribute__ ((aligned (128)));

char buf[] = {
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22,
	23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36
};

uint32_t select_f(float f1, float f2, float f3) {
	if (f1 < f2)
		return 1;
	if (f2 == f3)
		return 2;
	if (f2 + f3 > 0)
		return 3;
	return 4;
}

uint32_t select_u(int32_t f1, int32_t f2, int32_t f3) {
	if (f1 < f2)
		return 1;
	if (f2 == f3)
		return 2;
	if (f2 + f3 > 0)
		return 3;
	return 4;
}

int main(uint64_t spu_num, uint64_t u64s, uint64_t outputs, uint64_t message) 
{
	snprintf((char*)message_buffer, 128, "! %d !", 10);

	cellDmaGet(u64buffer, u64s, 64, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	cellDmaGet(outputsBuffer, outputs, sizeof(outputsBuffer), tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	uint8_t* u8s = (uint8_t*)outputsBuffer + 1;
	uint16_t* u16s = (uint16_t*)&u8s[3];
	uint32_t* u32s = (uint32_t*)&u8s[1];
	qword* v = (qword*)outputsBuffer;

	uint32_t bigqwbuf[4] = { 0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff };
	uint32_t bigqw1buf[4] = { 0x93780f0d, 0xc00aaa89, 0x23217598, 0x301abcde };
	uint32_t bigqw2buf[4] = { 0x82379487, 0x92138100, 0x94589494, 0x94949444 };

	qword bigqw =  *(qword*)bigqwbuf;
	qword bigqw1 = *(qword*)bigqw1buf;
	qword bigqw2 = *(qword*)bigqw2buf;

	float qwf1buf[4] = { 0.11, 5583.1223, 3.33, -0.1 };
	float qwf2buf[4] = { 1, 2.33, 3.33, -8 };
	qword qwf1 = *(qword*)&qwf1buf;
	qword qwf2 = *(qword*)&qwf2buf;

	double qwd1buf[2] = { 0.11, -2 };
	double qwd2buf[2] = { 0.11, 1 };
	qword qwd1 = *(qword*)&qwd1buf;
	qword qwd2 = *(qword*)&qwd2buf;
	
#define va (0xD0 + 16)
#define sva (0x10)
#define svaqw (qword)sva

	memcpy((void*)sva, buf, sizeof(buf));

	uint32_t val_0_buf[4] = { 0, 0, 0, 0 };
	uint32_t val_1_buf[4] = { 1, 1, 1, 1 };
	uint32_t val_n1_buf[4] = { -1, -1, -1, -1 };
	uint32_t val_3_buf[4] = { 3, 3, 3, 3 };
	uint32_t val_n7_buf[4] = { -7, -7, -7, -7 };
	uint32_t val_n64_buf[4] = { 64, 64, 64, 64 };
	uint32_t val_127_buf[4] = { 127, 127, 127, 127 };

	qword val_0 = *(qword*)val_0_buf;
	qword val_1 = *(qword*)val_1_buf;
	qword val_n1 = *(qword*)val_n1_buf;
	qword val_3 = *(qword*)val_3_buf;
	qword val_n7 = *(qword*)val_n7_buf;
	qword val_n64 = *(qword*)val_n64_buf;
	qword val_127 = *(qword*)val_127_buf;

	int i = 0;

	v[i] = si_cbd(v[i], 1);
	v[i + 1] = si_cbd(v[i + 1], 7);
	v[i + 2] = si_cbd(v[i + 2], -64);
	v[i + 3] = si_cbd(v[i + 3], 63);

	i = 4;

	v[i] = si_cbx(v[i], v[i]);
	v[i + 1] = si_cbx(v[i + 1], v[i + 1]);
	v[i + 2] = si_cbx(v[i + 2], v[i + 2]);
	v[i + 3] = si_cbx(v[i + 3], v[i + 3]);

	i = 8;

	v[i] = si_cdd(v[i], 1);
	v[i + 1] = si_cdd(v[i + 1], 7);
	v[i + 2] = si_cdd(v[i + 2], -64);
	v[i + 3] = si_cdd(v[i + 3], 63);

	i = 12;

	v[i] = si_cdx(v[i], v[i]);
	v[i + 1] = si_cdx(v[i + 1], v[i + 1]);
	v[i + 2] = si_cdx(v[i + 2], v[i + 2]);
	v[i + 3] = si_cdx(v[i + 3], v[i + 3]);

	i = 16;

	v[i] = si_chd(v[i], 1);
	v[i + 1] = si_chd(v[i + 1], 7);
	v[i + 2] = si_chd(v[i + 2], -64);
	v[i + 3] = si_chd(v[i + 3], 63);

	i = 20;

	v[i] = si_chx(v[i], v[i]);
	v[i + 1] = si_chx(v[i + 1], v[i + 1]);
	v[i + 2] = si_chx(v[i + 2], v[i + 2]);
	v[i + 3] = si_chx(v[i + 3], v[i + 3]);

	i = 24;

	v[i] = si_cwd(v[i], 1);
	v[i + 1] = si_cwd(v[i + 1], 7);
	v[i + 2] = si_cwd(v[i + 2], -64);
	v[i + 3] = si_cwd(v[i + 3], 63);

	i = 28;

	v[i] = si_cwx(v[i], v[i]);
	v[i + 1] = si_cwx(v[i + 1], v[i + 1]);
	v[i + 2] = si_cwx(v[i + 2], v[i + 2]);
	v[i + 3] = si_cwx(v[i + 3], v[i + 3]);

	i = 32;

	v[i] = si_il(0);
	v[i + 1] = si_il(-77);
	v[i + 2] = si_il(-32768);
	v[i + 3] = si_il(32767);

	i = 36;

	v[i] = si_ila(0);
	v[i + 1] = si_ila(3333);
	v[i + 2] = si_ila(262143);
	v[i + 3] = si_ila(800);

	i = 40;

	v[i] = si_ilh(0);
	v[i + 1] = si_ilh(-77);
	v[i + 2] = si_ilh(-32768);
	v[i + 3] = si_ilh(32767);

	i = 44;

	v[i] = si_ilhu(0);
	v[i + 1] = si_ilhu(-77);
	v[i + 2] = si_ilhu(-32768);
	v[i + 3] = si_ilhu(32767);

	i = 48;

	v[i] = si_iohl(v[i], 0);
	v[i + 1] = si_iohl(v[i + 1], 500);
	v[i + 2] = si_iohl(v[i + 2], 8993);
	v[i + 3] = si_iohl(v[i + 3], 65535);

	i = 52;

	v[i] = si_lqa(va);
	v[i + 1] = si_lqa(va + 1);
	v[i + 2] = si_lqa(va + 3);
	v[i + 3] = si_lqa(va + 7);

	i = 56;

	v[i] = si_lqd(val_0, va);
	v[i + 1] = si_lqd(val_n1, va);
	v[i + 2] = si_lqd(val_3, va);
	v[i + 3] = si_lqd(val_n7, va);

	i = 60;

	v[i] =     si_lqr(va);
	v[i + 1] = si_lqr(va + 1);
	v[i + 2] = si_lqr(va + 3);
	v[i + 3] = si_lqr(va + 7);

	i = 64;

	v[i] =     si_lqx(val_0, svaqw);
	v[i + 1] = si_lqx(val_n1, svaqw);
	v[i + 2] = si_lqx(val_3, svaqw);
	v[i + 3] = si_lqx(val_n7, svaqw);

	i = 68;

	memcpy((void*)sva, buf, sizeof(buf));

	v[i] =     (si_stqa(v[i],     sva), si_lqa(sva));
	v[i + 1] = (si_stqa(v[i + 1], sva), si_lqa(sva));
	v[i + 2] = (si_stqa(v[i + 2], sva), si_lqa(sva));
	v[i + 3] = (si_stqa(v[i + 3], sva), si_lqa(sva));

	i = 72;

	memcpy((void*)sva, buf, sizeof(buf));

	v[i] =     (si_stqd(v[i],     val_0, sva), si_lqd(val_0, sva));
	v[i + 1] = (si_stqd(v[i + 1], val_0, sva), si_lqd(val_0, sva));
	v[i + 2] = (si_stqd(v[i + 2], val_3, sva), si_lqd(val_3, sva));
	v[i + 3] = (si_stqd(v[i + 3], val_3, sva), si_lqd(val_3, sva));

	i = 76;

	memcpy((void*)sva, buf, sizeof(buf));

	v[i] =     (si_stqr(v[i],     sva), si_lqr(sva));
	v[i + 1] = (si_stqr(v[i + 1], sva), si_lqr(sva));
	v[i + 2] = (si_stqr(v[i + 2], sva), si_lqr(sva));
	v[i + 3] = (si_stqr(v[i + 3], sva), si_lqr(sva));

	i = 80;

	memcpy((void*)sva, buf, sizeof(buf));

	v[i] =     (si_stqx(v[i],     val_0, svaqw), si_lqx(val_0, svaqw));
	v[i + 1] = (si_stqx(v[i + 1], val_0, svaqw), si_lqx(val_0, svaqw));
	v[i + 2] = (si_stqx(v[i + 2], val_3, svaqw), si_lqx(val_3, svaqw));
	v[i + 3] = (si_stqx(v[i + 3], val_3, svaqw), si_lqx(val_3, svaqw));

	i = 84;

	v[i++] = (qword)si_to_char(bigqw);
	v[i++] = (qword)si_to_uchar(bigqw);
	v[i++] = (qword)si_to_short(bigqw);
	v[i++] = (qword)si_to_ushort(bigqw);
	v[i++] = (qword)si_to_int(bigqw);
	v[i++] = (qword)si_to_uint(bigqw);
	v[i++] = (qword)(uint32_t)si_to_ptr(bigqw);
	v[i++] = (qword)si_to_llong(bigqw);
	v[i++] = (qword)si_to_ullong(bigqw);
	v[i++] = (qword)si_to_float(bigqw);
	v[i++] = (qword)si_to_double(bigqw);

	i = 95;

	v[i++] = si_from_char(0x13);
	v[i++] = si_from_uchar(0x4199);
	v[i++] = si_from_short(0x77ff);
	v[i++] = si_from_ushort(0xff77);
	v[i++] = si_from_int(0x13131313);
	v[i++] = si_from_uint(0xf7f7f7f7);
	v[i++] = si_from_ptr((void*)0x444);
	v[i++] = si_from_llong(0xffaaffaa11221122ull);
	v[i++] = si_from_ullong(0xffaaffaa11221122ull);
	v[i++] = si_from_float(0.11);
	v[i++] = si_from_double(0.13);

	i = 106;

	volatile unsigned char uc = 0x13;
	volatile signed char sc = 0x99;
	volatile unsigned short us = 0x77ff;
	volatile signed short ss = 0xff77;
	volatile unsigned int ui = 0x13131313;
	volatile signed int si = 0xf7f7f7f7;
	volatile unsigned long long ull = 0xffaaffaa11221122ull;
	volatile signed long long sll = 0xffaaffaa11221122ull;
	volatile float f = 0.11;
	volatile double d = 0.11;

	v[i++] = (qword)spu_splats(uc);
	v[i++] = (qword)spu_splats(sc);
	v[i++] = (qword)spu_splats(us);
	v[i++] = (qword)spu_splats(ss);
	v[i++] = (qword)spu_splats(ui);
	v[i++] = (qword)spu_splats(si);
	v[i++] = (qword)spu_splats(ull);
	v[i++] = (qword)spu_splats(sll);
	v[i++] = (qword)spu_splats(f);
	v[i++] = (qword)spu_splats(d);

	i = 116;

	v[i++] = (qword)spu_splats((unsigned char)0x13);
	v[i++] = (qword)spu_splats((signed char)0x4199);
	v[i++] = (qword)spu_splats((unsigned short)0x77ff);
	v[i++] = (qword)spu_splats((signed short)0xff77);
	v[i++] = (qword)spu_splats((unsigned int)0x13131313);
	v[i++] = (qword)spu_splats((signed int)0xf7f7f7f7);
	v[i++] = (qword)spu_splats((unsigned long long)0xffaaffaa11221122ull);
	v[i++] = (qword)spu_splats((signed long long)0xffaaffaa11221122ull);
	v[i++] = (qword)spu_splats((float)0.11);
	v[i++] = (qword)spu_splats((double)0.11);

	i = 126;

	v[i++] = si_cuflt(val_0, 0);
	v[i++] = si_cuflt(val_1, 1);
	//v[i++] = si_cuflt(val_n1, 10);
	//v[i++] = si_csflt(val_n7, 10);
	//v[i++] = si_csflt(val_3, 10);
	v[i++] = si_csflt(val_n64, 10);

	i = 132;

	v[i++] = si_cflts(qwf1, 0);
	v[i++] = si_cflts(qwf1, 4);
	v[i++] = si_cflts(qwf1, 2);

	i = 135;

	v[i++] = si_cfltu(qwf1, 0);
	v[i++] = si_cfltu(qwf1, 2);
	v[i++] = si_cfltu(qwf1, 1);

	i = 138;

	v[i++] = si_xsbh(bigqw2);
	v[i++] = si_xshw(bigqw2);
	v[i++] = si_xswd(bigqw1);
	v[i++] = si_fesd(qwf1);

	i = 142;

	v[i++] = si_frds(bigqw);

	i = 143;

	v[i++] = si_a(bigqw, bigqw1);
	v[i++] = si_ah(bigqw, bigqw1);
	v[i++] = si_ai(bigqw, 0);
	v[i++] = si_ai(bigqw, 510);
	v[i++] = si_ai(bigqw, 511);
	v[i++] = si_ai(bigqw, -512);
	v[i++] = si_ai(bigqw, -300);
	v[i++] = si_ahi(bigqw, 0);
	v[i++] = si_ahi(bigqw, 510);
	v[i++] = si_ahi(bigqw, -512);
	v[i++] = si_ahi(bigqw, -300);
	v[i++] = si_fa(bigqw, bigqw1);
	v[i++] = si_dfa(bigqw, bigqw1);

	i = 156;

	v[i++] = si_addx(bigqw, bigqw1, bigqw2);
	v[i++] = si_bg(bigqw, bigqw2);
	v[i++] = si_bgx(bigqw, bigqw1, bigqw2);
	v[i++] = si_cg(bigqw, bigqw1);
	v[i++] = si_cgx(bigqw, bigqw1, bigqw2);

	i = 161;

	v[i++] = si_mpya(bigqw, bigqw1, bigqw2);
	v[i++] = si_fma(bigqw, bigqw1, bigqw2);
	v[i++] = si_dfma(bigqw, bigqw1, bigqw2);
	v[i++] = si_mpyhha(bigqw, bigqw1, bigqw2);
	v[i++] = si_mpyhhau(bigqw, bigqw1, bigqw2);

	i = 166;

	v[i++] = si_fms(bigqw, bigqw1, bigqw2);
	v[i++] = si_dfms(qwd1, qwd1, qwd2);
	v[i++] = si_fm(qwf1, qwf2);
	v[i++] = si_dfm(qwd1, qwd2);
	v[i++] = si_mpyh(bigqw, bigqw1);
	v[i++] = si_mpyhh(bigqw, bigqw1);
	v[i++] = si_mpyhhu(bigqw, bigqw1);

	i = 173;

	v[i++] = si_mpy(bigqw, bigqw1);
	v[i++] = si_mpyi(bigqw, 0);
	v[i++] = si_mpyi(bigqw, -512);
	v[i++] = si_mpyi(bigqw, 511);
	v[i++] = si_mpyi(bigqw, 200);
	v[i++] = si_mpyi(bigqw, -300);

	v[i++] = si_mpyu(bigqw, bigqw1);
	v[i++] = si_mpyui(bigqw, 0);
	v[i++] = si_mpyui(bigqw, -512);
	v[i++] = si_mpyui(bigqw, 511);
	v[i++] = si_mpyui(bigqw, 200);
	v[i++] = si_mpyui(bigqw, -300);

	i = 185;

	v[i++] = si_mpys(bigqw, bigqw1);
	v[i++] = si_dfnma(qwd1, qwd1, qwd2);
	v[i++] = si_fnms(qwf1, qwf2, qwf1);
	v[i++] = si_dfnms(qwd1, qwd1, qwd1);
	v[i++] = si_frest(qwf1);
	v[i++] = si_fi(qwf1, qwf2);

	i = 191;

	v[i++] = si_frsqest(qwf1);
	v[i++] = si_fi(qwf1, qwf2);

	i = 193;

	v[i++] = si_sfh(bigqw, bigqw2);
	v[i++] = si_sf(bigqw, bigqw2);
	v[i++] = si_sfi(bigqw, 0);
	v[i++] = si_sfi(bigqw, 511);
	v[i++] = si_sfi(bigqw, -512);
	v[i++] = si_sfi(bigqw, 300);
	v[i++] = si_sfhi(bigqw, 0);
	v[i++] = si_sfhi(bigqw, 511);
	v[i++] = si_sfhi(bigqw, -512);
	v[i++] = si_sfhi(bigqw, 300);
	v[i++] = si_fs(qwf1, qwf2);
	v[i++] = si_dfs(qwd1, qwd2);

	i = 205;

	v[i++] = si_sfx(bigqw, bigqw2, bigqw1);

	i = 206;

	v[i++] = si_absdb(bigqw, bigqw2);
	v[i++] = si_avgb(bigqw, bigqw2);
	v[i++] = si_sumb(bigqw, bigqw2);

	i = 209;

	v[i++] = si_fcmgt(qwf1, qwf2);
	v[i++] = si_fcmgt(qwf2, qwf2);
	//v[i++] = (qword)si_dfcmgt(bigqw, bigqw2);
	//v[i++] = (qword)si_dfcmgt(bigqw, bigqw);
	v[i++] = si_ceqb(bigqw, bigqw2);
	v[i++] = si_ceqb(bigqw, bigqw);
	v[i++] = si_ceqh(bigqw, bigqw2);
	v[i++] = si_ceqh(bigqw, bigqw);
	v[i++] = si_ceq(bigqw, bigqw2);
	v[i++] = si_ceq(bigqw, bigqw);
	v[i++] = si_fceq(qwf1, qwf2);
	v[i++] = si_fceq(qwf1, qwf1);
	//v[i++] = (qword)si_dfceq(bigqw, bigqw2);
	//v[i++] = (qword)si_dfceq(bigqw, bigqw);

	i = 223;

	v[i++] = si_ceqbi(bigqw, 0);
	v[i++] = si_ceqbi(bigqw, 0x22);
	v[i++] = si_ceqhi(bigqw, 0);
	v[i++] = si_ceqhi(bigqw, 0x23);
	v[i++] = si_ceqi(bigqw, 0);
	v[i++] = si_ceqi(bigqw, 0x47);

	i = 229;

	v[i++] = si_cgtb(bigqw, bigqw2);
	v[i++] = si_cgtb(bigqw1, bigqw2);
	v[i++] = si_clgtb(bigqw, bigqw2);
	v[i++] = si_clgtb(bigqw1, bigqw2);
	v[i++] = si_cgth(bigqw, bigqw2);
	v[i++] = si_cgth(bigqw1, bigqw2);
	v[i++] = si_cgt(bigqw, bigqw2);
	v[i++] = si_cgt(bigqw1, bigqw2);
	v[i++] = si_clgt(bigqw, bigqw2);
	v[i++] = si_clgt(bigqw1, bigqw2);
	v[i++] = si_fcgt(bigqw, bigqw2);
	v[i++] = si_fcgt(bigqw1, bigqw2);
	//v[i++] = (qword)si_dcgtb(bigqw, bigqw2);
	//v[i++] = (qword)si_dcgtb(bigqw1, bigqw2);

	i = 243;

	v[i++] = si_cgtbi(bigqw, 17);
	v[i++] = si_cgtbi(bigqw1, 25);
	v[i++] = si_clgtbi(bigqw, -10);
	v[i++] = si_clgtbi(bigqw1, 100);
	v[i++] = si_cgthi(bigqw, 140);
	v[i++] = si_cgthi(bigqw1, -33);
	v[i++] = si_cgti(bigqw, 17);
	v[i++] = si_cgti(bigqw1, 7);
	v[i++] = si_clgti(bigqw, 333);
	v[i++] = si_clgti(bigqw1, 0);
	//v[i++] = (qword)si_fcgti(bigqw, 0);
	//v[i++] = (qword)si_fcgti(bigqw1, -1);

	i = 255;

	v[i++] = si_cntb(bigqw);
	v[i++] = si_clz(bigqw);
	v[i++] = si_gbb(bigqw);
	v[i++] = si_gbh(bigqw);
	v[i++] = si_gb(bigqw);
	
	i = 260;

	v[i++] = si_fsmb(bigqw);
	v[i++] = si_fsmbi(0xff11);

	i = 262;

	v[i++] = si_fsmh(bigqw);
	v[i++] = si_fsm(bigqw);
	v[i++] = si_selb(bigqw, bigqw1, bigqw2);
	v[i++] = si_shufb(bigqw, bigqw1, bigqw);
	v[i++] = si_shufb(bigqw, bigqw1, bigqw1);
	v[i++] = si_shufb(bigqw, bigqw1, bigqw2);
	v[i++] = si_shufb(bigqw, bigqw1, val_0);

	i = 269;

	v[i++] = si_and(bigqw, bigqw1);
	v[i++] = si_andbi(bigqw, 155);
	v[i++] = si_andhi(bigqw, 255);
	v[i++] = si_andi(bigqw, 511);
	v[i++] = si_andc(bigqw, bigqw1);
	v[i++] = si_eqv(bigqw, bigqw1);
	v[i++] = si_eqv(bigqw1, bigqw1);
	v[i++] = si_nand(bigqw1, bigqw2);
	v[i++] = si_nor(bigqw1, bigqw2);

	i = 278;

	v[i++] = si_or(bigqw, bigqw1);
	v[i++] = si_orbi(bigqw, 155);
	v[i++] = si_orhi(bigqw, 255);
	v[i++] = si_ori(bigqw, 511);
	v[i++] = si_orc(bigqw, bigqw1);
	v[i++] = si_orx(bigqw);

	i = 284;

	v[i++] = si_xor(bigqw, bigqw1);
	v[i++] = si_xorbi(bigqw, 155);
	v[i++] = si_xorhi(bigqw, 255);
	v[i++] = si_xori(bigqw, 511);

	i = 288;

	v[i++] = si_roth(bigqw, bigqw2);
	v[i++] = si_rot(bigqw, bigqw2);
	v[i++] = si_rothi(bigqw, 0);
	v[i++] = si_rothi(bigqw, 50);
	v[i++] = si_roti(bigqw, 5);
	v[i++] = si_roti(bigqw, 0);

	i = 294;

	v[i++] = si_rothm(bigqw, bigqw2);
	v[i++] = si_rotm(bigqw, bigqw2);
	v[i++] = si_rothmi(bigqw, 77);
	v[i++] = si_rotmi(bigqw, 8);

	i = 298;

	v[i++] = si_rotmah(bigqw, bigqw2);
	v[i++] = si_rotma(bigqw, val_1);
	v[i++] = si_rotmahi(bigqw2, 2);
	v[i++] = si_rotmai(bigqw1, 0);

	i = 302;

	v[i++] = si_rotqmbii(bigqw, 0);
	v[i++] = si_rotqmbii(bigqw, -1);
	v[i++] = si_rotqmbii(bigqw, 7);
	v[i++] = si_rotqmbii(bigqw, 20);
	v[i++] = si_rotqmbi(bigqw, bigqw2);
	v[i++] = si_rotqmbi(bigqw, val_0);
	v[i++] = si_rotqmbi(bigqw, bigqw1);
	v[i++] = si_rotqmbi(bigqw, val_0);

	i = 310;
	
	v[i++] = si_rotqmbyi(bigqw, 0);
	v[i++] = si_rotqmbyi(bigqw, -1);
	v[i++] = si_rotqmbyi(bigqw, 7);
	v[i++] = si_rotqmbyi(bigqw, 20);
	v[i++] = si_rotqmby(bigqw, bigqw2);
	v[i++] = si_rotqmby(bigqw, val_0);
	v[i++] = si_rotqmby(bigqw, bigqw1);
	v[i++] = si_rotqmby(bigqw, val_0);

	i = 318;

	v[i++] = si_rotqmbybi(bigqw, val_0);
	v[i++] = si_rotqmbybi(bigqw, val_3);
	v[i++] = si_rotqmbybi(bigqw, val_n1);
	v[i++] = si_rotqmbybi(bigqw, bigqw2);

	i = 322;

	v[i++] = si_rotqbii(bigqw, 0);
	v[i++] = si_rotqbii(bigqw, -1);
	v[i++] = si_rotqbii(bigqw, 7);
	v[i++] = si_rotqbii(bigqw, 20);
	v[i++] = si_rotqbi(bigqw, bigqw2);
	v[i++] = si_rotqbi(bigqw, val_0);
	v[i++] = si_rotqbi(bigqw, bigqw1);
	v[i++] = si_rotqbi(bigqw, val_0);

	i = 330;

	v[i++] = si_rotqbyi(bigqw, 0);
	v[i++] = si_rotqbyi(bigqw, -1);
	v[i++] = si_rotqbyi(bigqw, 7);
	v[i++] = si_rotqbyi(bigqw, 20);
	v[i++] = si_rotqby(bigqw, bigqw2);
	v[i++] = si_rotqby(bigqw, val_0);
	v[i++] = si_rotqby(bigqw, bigqw1);
	v[i++] = si_rotqby(bigqw, val_0);

	i = 338;

	v[i++] = si_rotqbybi(bigqw, val_0);
	v[i++] = si_rotqbybi(bigqw, val_3);
	v[i++] = si_rotqbybi(bigqw, val_n1);
	v[i++] = si_rotqbybi(bigqw, bigqw2);

	i = 342;

	v[i++] = si_shlh(bigqw, bigqw2);
	v[i++] = si_shlh(bigqw, bigqw);
	v[i++] = si_shlh(bigqw, bigqw1);
	v[i++] = si_shl(bigqw, bigqw);
	v[i++] = si_shl(bigqw, bigqw1);
	v[i++] = si_shl(bigqw, bigqw2);
	v[i++] = si_shlhi(bigqw, 0);
	v[i++] = si_shlhi(bigqw, 1);
	v[i++] = si_shlhi(bigqw, -1);
	v[i++] = si_shlhi(bigqw, 7);
	v[i++] = si_shli(bigqw, 0);
	v[i++] = si_shli(bigqw, 1);
	v[i++] = si_shli(bigqw, -1);
	v[i++] = si_shli(bigqw, 7);

	i = 356;

	v[i++] = si_shlqbii(bigqw, 0);
	v[i++] = si_shlqbii(bigqw, 1);
	v[i++] = si_shlqbii(bigqw, -1);
	v[i++] = si_shlqbii(bigqw, -64);
	v[i++] = si_shlqbii(bigqw, 127);

	v[i++] = si_shlqbi(bigqw, val_0);
	v[i++] = si_shlqbi(bigqw, val_1);
	v[i++] = si_shlqbi(bigqw, val_n1);
	v[i++] = si_shlqbi(bigqw, val_n64);
	v[i++] = si_shlqbi(bigqw, val_127);

	i = 366;

	v[i++] = si_shlqbyi(bigqw, 0);
	v[i++] = si_shlqbyi(bigqw, 1);
	v[i++] = si_shlqbyi(bigqw, -1);
	v[i++] = si_shlqbyi(bigqw, -64);
	v[i++] = si_shlqbyi(bigqw, 127);

	v[i++] = si_shlqby(bigqw, val_0);
	v[i++] = si_shlqby(bigqw, val_1);
	v[i++] = si_shlqby(bigqw, val_n1);
	v[i++] = si_shlqby(bigqw, val_n64);
	v[i++] = si_shlqby(bigqw, val_127);

	i = 376;

	v[i++] = si_shlqbybi(bigqw, val_0);
	v[i++] = si_shlqbybi(bigqw, val_1);
	v[i++] = si_shlqbybi(bigqw, val_n1);
	v[i++] = si_shlqby(bigqw, val_n64);
	v[i++] = si_shlqbybi(bigqw, val_127);

	i = 381;

	uint32_t select_f_buf[4] = {
		select_f(-0.5, 0.5, 0),
		select_f(2, 1, 1),
		select_f(2, 2, 3),
		select_f(2, 2, -3)
	};

	uint32_t select_u_buf[4] = {
		select_u(-5, 5, 0),
		select_u(2, 1, 1),
		select_u(2, 2, 3),
		select_u(2, 2, -3)
	};

	v[i++] = *(qword*)select_f_buf;
	v[i++] = *(qword*)select_u_buf;

	cellDmaPut(message_buffer, message, 128, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	cellDmaPut(outputsBuffer, outputs, sizeof(outputsBuffer), tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	return 0;
}
