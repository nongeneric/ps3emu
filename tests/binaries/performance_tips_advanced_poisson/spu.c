/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include <cell/spurs.h>
#include <cell/sync.h>
#include <cell/dma.h>
#include <spu_printf.h>
#include <spu_intrinsics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define TAG 1
#define VECTOR
float in_buf[8][NY] __attribute__((aligned(128)));

DataAddress arg;
ScalarData scalar_data;
ScalarData scalar_data_list[POISSON_SPU_NUM];

void getLine(float buf[NY], uint64_t ea, int nx, int tag);
void putLine(float buf[NY], uint64_t ea, int nx, int tag);
void printLine(float x[]);

void getLine(float buf[NY], uint64_t ea, int nx, int tag)
{
	cellDmaGetf(buf, ea + (nx * NY) * sizeof(float), NY * sizeof(float), tag, 0, 0);
}

void putLine(float buf[NY], uint64_t ea, int nx, int tag)
{
	cellDmaPutf(buf, ea + (nx * NY) * sizeof(float), NY * sizeof(float), tag, 0, 0);
}

void printLine(float x[])
{
	for (int i = 0; i < NY; i++) {
		spu_printf("%01.2f ", x[i]);
	}

	spu_printf("\n");
}

void calcAlpha(float beta, float *p_pr, float *p_pap, int p_load_id, int start_line, int end_line);
void calcBeta(float alpha, float *p_rap, float *p_rr, int p_load_id, int start_line, int end_line);

#define rotate_buf4(a, b, c, d) { int t = a; a = b; b = c; c = d; d = t; }
#define rotate_buf2(a, b)		{ int t = a; a = b; b = t; }

static inline void waitSync(uint64_t barrier)
{
	int ret;
	do
	{
		ret = cellSyncBarrierTryNotify(barrier);
	} while (ret != CELL_OK);
	do
	{
		ret = cellSyncBarrierTryWait(barrier);
	} while (ret != CELL_OK);
}

int cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
	(void)argTask;
	(void)argTaskset;

	uint64_t ea = spu_extract((vec_ullong2) argTask, 0);
	uint32_t spu_id = spu_extract((vec_uint4) argTask, 2);

	cellDmaLargeGet(&arg, ea, sizeof(DataAddress), TAG, 0, 0);
	cellDmaWaitTagStatusAll(1 << TAG);

	// --- calculate alpha --------------------------------------------
	uint32_t start_time;
	uint32_t end_time;

	float alpha;
	float beta;
	float rr;

	// initialize
	alpha = 1.0f;
	beta = 0.0f;

	int counter = 0;
	int start_x, end_x;
	start_x = NX * spu_id / POISSON_SPU_NUM;
	end_x = NX * (spu_id + 1) / POISSON_SPU_NUM;
	start_time = spu_read_decrementer();

	do
	{
		counter++;

		int input_p_buf = counter & 1;
		float pr, pap, rap;
		calcAlpha(beta, &pr, &pap, input_p_buf, start_x, end_x);

		// sync
		scalar_data.pap = pap;
		scalar_data.pr = pr;
		cellDmaPut(&scalar_data, arg.scalar_ea + sizeof(ScalarData) * spu_id, sizeof(ScalarData), TAG, 0, 0);
		cellDmaWaitTagStatusAll(1 << TAG);
		waitSync(arg.sync_ea);
		cellDmaGet(scalar_data_list, arg.scalar_ea, sizeof(ScalarData), TAG, 0, 0);
		cellDmaWaitTagStatusAll(1 << TAG);
		pap = 0;
		pr = 0;
		for (int i = 0; i < POISSON_SPU_NUM; i++) {
			pap += scalar_data_list[i].pap;
			pr += scalar_data_list[i].pr;
		}

		alpha = pr / pap;

		calcBeta(alpha, &rap, &rr, 1 - input_p_buf, start_x, end_x);

		// sync
		scalar_data.rap = rap;
		scalar_data.rr = rr;
		cellDmaPut(&scalar_data, arg.scalar_ea + sizeof(ScalarData) * spu_id, sizeof(ScalarData), TAG, 0, 0);
		cellDmaWaitTagStatusAll(1 << TAG);
		waitSync(arg.sync_ea);
		cellDmaGet(scalar_data_list, arg.scalar_ea, sizeof(ScalarData), TAG, 0, 0);
		cellDmaWaitTagStatusAll(1 << TAG);
		rap = 0;
		rr = 0;
		for (int i = 0; i < POISSON_SPU_NUM; i++) {
			rap += scalar_data_list[i].rap;
			rr += scalar_data_list[i].rr;
		}

		beta = -rap / pap;
	} while (rr > MAX_RESIDUAL);
	end_time = spu_read_decrementer();

	if (spu_id == 0) {
		spu_printf("%d SPUs, Cycle %d, rr:%f\n", POISSON_SPU_NUM, counter, rr);
		spu_printf("Time:%f msec / step\n", (start_time - end_time) / counter / 80000.0f);
	}

	return 0;
}

void calcAlpha(float beta, float *p_pr, float *p_pap, int p_load_id, int start_line, int end_line)
{
	float pr = 0;
	vec_float4 vec_pr = { 0, 0, 0, 0 };
	float pap = 0;
	int nx = 0;
	int p_prev_buf = 0;
	int p_curr_buf = 1;
	int p_next_buf = 2;
	int p_load_buf = 3;
	int r_curr_buf = 4;
	int r_load_buf = 5;
	int ap_curr_buf = 6;
	int ap_store_buf = 7;

	// get line
	// prepare prev p
	if (start_line > 0) {
		getLine(in_buf[p_prev_buf], arg.p_ea[p_load_id], start_line - 1, TAG);
		getLine(in_buf[r_curr_buf], arg.r_ea, start_line - 1, TAG);
		cellDmaWaitTagStatusAll(1 << TAG);
		for (int i = 0; i < NY; i++) {
			in_buf[p_prev_buf][i] = in_buf[r_curr_buf][i] + beta * in_buf[p_prev_buf][i];
		}
	}

	getLine(in_buf[p_curr_buf], arg.p_ea[p_load_id], start_line, TAG);
	getLine(in_buf[p_next_buf], arg.p_ea[p_load_id], start_line + 1, TAG);
	getLine(in_buf[r_curr_buf], arg.r_ea, start_line + 1, TAG);
	getLine(in_buf[r_load_buf], arg.r_ea, start_line, TAG);
	cellDmaWaitTagStatusAll(1 << TAG);

	// calc new p
	for (int i = 0; i < NY; i++) {
		in_buf[p_curr_buf][i] = in_buf[r_load_buf][i] + beta * in_buf[p_curr_buf][i];
		pr += in_buf[p_curr_buf][i] * in_buf[r_load_buf][i];
	}

	if (start_line == 0) {
		// set zero
		memset(in_buf[p_prev_buf], 0, NY * sizeof(float));
	}

	//printLine(in_buf[p_prev_buf]);
	//printLine(in_buf[p_curr_buf]);
	//printLine(in_buf[p_next_buf]);
	//printLine(in_buf[r_curr_buf]);
	for (nx = start_line; nx < end_line; nx++) {
		int tag = (nx + 1) & 1;

		// kick DMA get
		if (nx < NX - 2) {
			getLine(in_buf[p_load_buf], arg.p_ea[p_load_id], nx + 2, tag);
			getLine(in_buf[r_load_buf], arg.r_ea, nx + 2, tag);
		}

		// wait done
		cellDmaWaitTagStatusAll(1 << (1 - tag));

		uint32_t i = 0;

		// calc new p
		if (nx + 1 < end_line)
		{
#ifdef VECTOR
			vec_float4 *p_next = (vec_float4 *) &in_buf[p_next_buf][0];
			vec_float4 *r_curr = (vec_float4 *) &in_buf[r_curr_buf][0];

			for (i = 0; i < NY; i += 8) {
				*(p_next + 0) = spu_madd(spu_splats(beta), *(p_next + 0), *(r_curr + 0));
				*(p_next + 1) = spu_madd(spu_splats(beta), *(p_next + 1), *(r_curr + 1));
				vec_pr = spu_madd(*(p_next + 0), *(r_curr + 0), vec_pr);
				vec_pr = spu_madd(*(p_next + 1), *(r_curr + 1), vec_pr);
				p_next += 2;
				r_curr += 2;
			}

#else // VECTOR
			for (i = 0; i < NY; i++) {
				in_buf[p_next_buf][i] = in_buf[r_curr_buf][i] + beta * in_buf[p_next_buf][i];
				pr += in_buf[p_next_buf][i] * in_buf[r_curr_buf][i];
			}
#endif // VECTOR
		}
		else if (nx + 1 < NX) {
			for (i = 0; i < NY; i++) {
				in_buf[p_next_buf][i] = in_buf[r_curr_buf][i] + beta * in_buf[p_next_buf][i];
			}
		}
		else {
			memset(in_buf[p_next_buf], 0, NY * sizeof(float));
		}

#ifdef VECTOR
		const vector unsigned char pattern_l1r3 = { 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b };
		const vector unsigned char pattern_l3r1 = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13 };

		i = 0;

		vec_float4 vec_pap = { 0, 0, 0, 0 };

		// first element
		vec_float4 left0, center0, top0, bottom0, right0;
		vec_float4 left1, center1, top1, bottom1, right1;

		// boundary condition
		left0 = spu_splats(0.0f);
		center0 = *(vec_float4 *) &in_buf[p_curr_buf][0];
		top0 = *(vec_float4 *) &in_buf[p_prev_buf][0];
		bottom0 = *(vec_float4 *) &in_buf[p_next_buf][0];
		right0 = *(vec_float4 *) &in_buf[p_curr_buf][4];
		for (i = 0; i < NY; i += 8) {
			left1 = *(vec_float4 *) &in_buf[p_curr_buf][i];
			center1 = *(vec_float4 *) &in_buf[p_curr_buf][i + 4];
			top1 = *(vec_float4 *) &in_buf[p_prev_buf][i + 4];
			bottom1 = *(vec_float4 *) &in_buf[p_next_buf][i + 4];
			if (__builtin_expect(i + 8 < NY, 1)) {
				right1 = *(vec_float4 *) &in_buf[p_curr_buf][i + 8];
			}
			else {
				// boundary condition
				right1 = spu_splats(0.0f);
			}

			// shift
			left0 = spu_shuffle(left0, center0, pattern_l1r3);
			right0 = spu_shuffle(center0, right0, pattern_l3r1);

			// calculation
			vec_float4 horizontal0 = spu_add(right0, left0);
			vec_float4 vertical0 = spu_add(top0, bottom0);
			vec_float4 new_ap0 = spu_add(horizontal0, vertical0);
			new_ap0 = spu_msub(new_ap0, spu_splats(0.25f), center0);
			vec_pap = spu_madd(new_ap0, center0, vec_pap);

			// latter
			if (__builtin_expect(i + 8 < NY, 1)) {
				left0 = *(vec_float4 *) &in_buf[p_curr_buf][i + 4];
				center0 = *(vec_float4 *) &in_buf[p_curr_buf][i + 8];
				top0 = *(vec_float4 *) &in_buf[p_prev_buf][i + 8];
				bottom0 = *(vec_float4 *) &in_buf[p_next_buf][i + 8];
				right0 = *(vec_float4 *) &in_buf[p_curr_buf][i + 12];
			}

			// shift
			left1 = spu_shuffle(left1, center1, pattern_l1r3);
			right1 = spu_shuffle(center1, right1, pattern_l3r1);

			// calculation
			vec_float4 horizontal1 = spu_add(right1, left1);
			vec_float4 vertical1 = spu_add(top1, bottom1);
			vec_float4 new_ap1 = spu_add(horizontal1, vertical1);
			new_ap1 = spu_msub(new_ap1, spu_splats(0.25f), center1);
			vec_pap = spu_madd(new_ap1, center1, vec_pap);

			*(vec_float4 *) &in_buf[ap_curr_buf][i] = new_ap0;
			*(vec_float4 *) &in_buf[ap_curr_buf][i + 4] = new_ap1;
		}

		pap += spu_extract(vec_pap, 0);
		pap += spu_extract(vec_pap, 1);
		pap += spu_extract(vec_pap, 2);
		pap += spu_extract(vec_pap, 3);

#else // VECTOR
		i = 0;

		// first element
		in_buf[ap_curr_buf][i] = (0 + in_buf[p_curr_buf][i + 1] + in_buf[p_next_buf][i] + in_buf[p_prev_buf][i]) * 0.25 - in_buf[p_curr_buf][i];
		pap += in_buf[ap_curr_buf][i] * in_buf[p_curr_buf][i];
		for (i = 1; i < NY - 1; i++) {
			in_buf[ap_curr_buf][i] = (in_buf[p_curr_buf][i - 1] + in_buf[p_curr_buf][i + 1] + in_buf[p_next_buf][i] + in_buf[p_prev_buf][i]) *
				0.25 - in_buf[p_curr_buf][i];
			pap += in_buf[ap_curr_buf][i] * in_buf[p_curr_buf][i];
		}

		// last element
		in_buf[ap_curr_buf][i] = (in_buf[p_curr_buf][i - 1] + 0 + in_buf[p_next_buf][i] + in_buf[p_prev_buf][i]) * 0.25 - in_buf[p_curr_buf][i];
		pap += in_buf[ap_curr_buf][i] * in_buf[p_curr_buf][i];
#endif // VECTOR
		putLine(in_buf[ap_curr_buf], arg.ap_ea, nx, (1 - tag));
		putLine(in_buf[p_curr_buf], arg.p_ea[1 - p_load_id], nx, (1 - tag));
		rotate_buf2(r_curr_buf, r_load_buf);
		rotate_buf2(ap_curr_buf, ap_store_buf);
		rotate_buf4(p_prev_buf, p_curr_buf, p_next_buf, p_load_buf);
	}

	pr += spu_extract(vec_pr, 0);
	pr += spu_extract(vec_pr, 1);
	pr += spu_extract(vec_pr, 2);
	pr += spu_extract(vec_pr, 3);
	*p_pr = pr;
	*p_pap = pap;

	// wait done
	cellDmaWaitTagStatusAll(1 << ((NX + 2) & 1));
}

void calcBeta(float alpha, float *p_rap, float *p_rr, int p_load_id, int start_line, int end_line)
{
	float rap = 0;
	float rr = 0;
	vec_float4 vec_rap = { 0, 0, 0, 0 };
	vec_float4 vec_rr = { 0, 0, 0, 0 };
	int nx = 0;
	int p_curr_buf = 0;
	int p_load_buf = 1;
	int x_curr_buf = 2;
	int x_load_buf = 3;
	int ap_curr_buf = 4;
	int ap_load_buf = 5;
	int r_curr_buf = 6;
	int r_load_buf = 7;

	// get line
	getLine(in_buf[p_load_buf], arg.p_ea[p_load_id], start_line, 0);
	getLine(in_buf[x_load_buf], arg.x_ea, start_line, 0);
	getLine(in_buf[r_load_buf], arg.r_ea, start_line, 0);
	getLine(in_buf[ap_load_buf], arg.ap_ea, start_line, 0);
	cellDmaWaitTagStatusAll(1 << 0);

	//printLine(in_buf[p_load_buf]);
	//printLine(in_buf[r_load_buf]);
	//printLine(in_buf[x_load_buf]);
	//printLine(in_buf[ap_load_buf]);
	for (nx = start_line; nx < end_line; nx++) {
		rotate_buf2(p_curr_buf, p_load_buf);
		rotate_buf2(x_curr_buf, x_load_buf);
		rotate_buf2(r_curr_buf, r_load_buf);
		rotate_buf2(ap_curr_buf, ap_load_buf);

		// kick DMA get
		int tag = (nx + 1) & 1;
		if (nx < NX - 1) {
			getLine(in_buf[p_load_buf], arg.p_ea[p_load_id], nx + 1, tag);
			getLine(in_buf[x_load_buf], arg.x_ea, nx + 1, tag);
			getLine(in_buf[r_load_buf], arg.r_ea, nx + 1, tag);
			getLine(in_buf[ap_load_buf], arg.ap_ea, nx + 1, tag);
		}

		// wait done
		cellDmaWaitTagStatusAll(1 << (1 - tag));

#ifdef VECTOR
		vec_float4 *p_x = (vec_float4 *) &in_buf[x_curr_buf][0];
		vec_float4 *p_r = (vec_float4 *) &in_buf[r_curr_buf][0];
		vec_float4 *p_p = (vec_float4 *) &in_buf[p_curr_buf][0];
		vec_float4 *p_ap = (vec_float4 *) &in_buf[ap_curr_buf][0];
		vec_float4 vec_alpha = spu_splats(alpha);
		vec_float4 vec_minus_alpha = spu_splats(-alpha);
		for (int i = 0; i < NY; i += 8) {
			vec_float4 x0 = *p_x;
			vec_float4 r0 = *p_r;
			vec_float4 p0 = *p_p;
			vec_float4 ap0 = *p_ap;
			vec_float4 x1 = *(p_x + 1);
			vec_float4 r1 = *(p_r + 1);
			vec_float4 p1 = *(p_p + 1);
			vec_float4 ap1 = *(p_ap + 1);
			x0 = spu_madd(vec_alpha, p0, x0);
			r0 = spu_madd(vec_minus_alpha, ap0, r0);
			vec_rap = spu_madd(r0, ap0, vec_rap);
			vec_rr = spu_madd(r0, r0, vec_rr);
			x1 = spu_madd(vec_alpha, p1, x1);
			r1 = spu_madd(vec_minus_alpha, ap1, r1);
			vec_rap = spu_madd(r1, ap1, vec_rap);
			vec_rr = spu_madd(r1, r1, vec_rr);
			*(p_x + 0) = x0;
			*(p_r + 0) = r0;
			*(p_x + 1) = x1;
			*(p_r + 1) = r1;
			p_x += 2;
			p_r += 2;
			p_p += 2;
			p_ap += 2;
		}

#else // VECTOR
		for (int i = 0; i < NY; i++) {
			in_buf[x_curr_buf][i] += alpha * in_buf[p_curr_buf][i];
			in_buf[r_curr_buf][i] -= alpha * in_buf[ap_curr_buf][i];
			rap += in_buf[r_curr_buf][i] * in_buf[ap_curr_buf][i];
			rr += in_buf[r_curr_buf][i] * in_buf[r_curr_buf][i];
		}
#endif // VECTOR
		putLine(in_buf[x_curr_buf], arg.x_ea, nx, (1 - tag));
		putLine(in_buf[r_curr_buf], arg.r_ea, nx, (1 - tag));
	}

#ifdef VECTOR
	rap += spu_extract(vec_rap, 0);
	rap += spu_extract(vec_rap, 1);
	rap += spu_extract(vec_rap, 2);
	rap += spu_extract(vec_rap, 3);
	rr += spu_extract(vec_rr, 0);
	rr += spu_extract(vec_rr, 1);
	rr += spu_extract(vec_rr, 2);
	rr += spu_extract(vec_rr, 3);
#endif // VECTOR
	int tag = (nx + 1) & 1;
	cellDmaWaitTagStatusAll(1 << (1 - tag));
	*p_rap = rap;
	*p_rr = rr;

}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
