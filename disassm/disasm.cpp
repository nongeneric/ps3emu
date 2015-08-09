// Copyright 2011 naehrwert
// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

//Parts of the disassembler from fail0verflow's anergistic.

#include <stdio.h>
#include <string.h>

#include "disasm.h"

constexpr int INSTR_SIZE = 4;

static inline u32 se(u32 v, int b)
{
    v <<= 32-b;
    v = ((s32)v) >> (32-b);
    return v;
}

static inline u32 se7(u32 v)
{
    return se(v, 7);
}

static inline u32 se10(u32 v)
{
    return se(v, 10);
}

static inline u32 se16(u32 v)
{
    return se(v, 16);
}

static inline u32 se18(u32 v)
{
    return se(v, 18);
}

static instr_t instr_cdd(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.
	i7 = se7(i7);

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_rotqmbii(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_clgt(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_nand(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_iohl(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_gb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpyhh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_cdx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_andbi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_orbi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_clz(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_absdb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_brhz(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_cntb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_stop(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_ceqi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_ceqh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_biz(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ceq(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ceqb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqbyi(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_nop(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_sumb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_nor(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpy(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_dsync(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_mpys(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_gbb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpyu(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_gbh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_roti(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_mpya(u32 rt, u32 ra, u32 rb, u32 rc)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rrr.rt = rt;
	res.rrr.ra = ra;
	res.rrr.rb = rb;
	res.rrr.rc = rc;

	return res;
}

static instr_t instr_rdch(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotm(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_xsbh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ilhu(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_cgti(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_mpyh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpyi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_shl(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_brsl(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_clgthi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_sync(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_heqi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_cwx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_xor(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqmbi(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_bihz(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ceqhi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_mpyhhau(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_avgb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_addx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqmby(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mfspr(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_stopd(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_xorhi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_cwd(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_bg(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_orx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_bi(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_cgx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_sfhi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_br(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_ori(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_andi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_orc(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ila(u32 rt, u32 i18)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri18.rt = rt;
	res.ri18.i18 = i18;

	return res;
}

static instr_t instr_xswd(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_ilh(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_bisl(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqmbyi(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_bgx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_or(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_hbr(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_brz(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_selb(u32 rt, u32 ra, u32 rb, u32 rc)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rrr.rt = rt;
	res.rrr.ra = ra;
	res.rrr.rb = rb;
	res.rrr.rc = rc;

	return res;
}

static instr_t instr_brhnz(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_ahi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_cg(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_hbrr(u32 rt, u32 i18)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri18.rt = rt;
	res.ri18.i18 = i18;

	return res;
}

static instr_t instr_mpyui(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_xori(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_fsmbi(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_shufb(u32 rt, u32 ra, u32 rb, u32 rc)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rrr.rt = rt;
	res.rrr.ra = ra;
	res.rrr.rb = rb;
	res.rrr.rc = rc;

	return res;
}

static instr_t instr_bihnz(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_bra(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_chd(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.
	i7 = se7(i7);

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_rchcnt(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_fsmh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpyhhu(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_xorbi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_lnop(u32 opcode)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.special.opcode = opcode;

	return res;
}

static instr_t instr_fsmb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_andc(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_eqv(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mpyhha(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotma(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_chx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rothmi(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_rotmi(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_clgtbi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_clgtb(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_clgti(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_clgth(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_shli(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_shlqbii(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_ceqbi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_shlqby(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_shlqbyi(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_shlqbi(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_and(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_stqd(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);
	i10 <<= 4;

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_cbd(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.
	i7 = se7(i7);

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_stqa(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_ai(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_ah(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqby(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_hbra(u32 rt, u32 i18)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri18.rt = rt;
	res.ri18.i18 = i18;

	return res;
}

static instr_t instr_stqr(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_il(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_cbx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_mtspr(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_stqx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_cgt(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_lqx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_lqr(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_wrch(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_lqd(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);
	i10 <<= 4;

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_cgthi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_rotmai(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

static instr_t instr_lqa(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_sfx(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_brnz(u32 rt, u32 i16)
{
	instr_t res;
	
	//Pretransform.
	i16 = se16(i16);
	i16 <<= 2;

	//Set args.
	res.ri16.rt = rt;
	res.ri16.i16 = i16;

	return res;
}

static instr_t instr_andhi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.
	i10 = se10(i10);

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_orhi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_sfh(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_sfi(u32 rt, u32 ra, u32 i10)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri10.rt = rt;
	res.ri10.ra = ra;
	res.ri10.i10 = i10;

	return res;
}

static instr_t instr_xshw(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_a(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_fsm(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_binz(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_sf(u32 rt, u32 ra, u32 rb)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.rr.rt = rt;
	res.rr.ra = ra;
	res.rr.rb = rb;

	return res;
}

static instr_t instr_rotqbii(u32 rt, u32 ra, u32 i7)
{
	instr_t res;
	
	//Pretransform.

	//Set args.
	res.ri7.rt = rt;
	res.ri7.ra = ra;
	res.ri7.i7 = i7;

	return res;
}

#define INSTRUCTION_ENTRY(a, b, c, d) {a, b, c, (void*)d}

static const struct {
	const char *name;
	int type;
	int instr;
	void *ptr;
} instr_tbl[] =
{
	INSTRUCTION_ENTRY("stop", SPU_INSTR_SPECIAL, INSTR_STOP, instr_stop), // 00000000
	INSTRUCTION_ENTRY("lnop", SPU_INSTR_SPECIAL, INSTR_LNOP, instr_lnop), // 02000000
	INSTRUCTION_ENTRY("sync", SPU_INSTR_SPECIAL, INSTR_SYNC, instr_sync), // 04000000
	INSTRUCTION_ENTRY("dsync", SPU_INSTR_SPECIAL, INSTR_DSYNC, instr_dsync), // 06000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 08000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 0a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 0c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 0e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 10000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 12000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 14000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 16000000
	INSTRUCTION_ENTRY("mfspr", SPU_INSTR_RR, INSTR_MFSPR, instr_mfspr), // 18000000
	INSTRUCTION_ENTRY("rdch", SPU_INSTR_RR, INSTR_RDCH, instr_rdch), // 1a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1c000000
	INSTRUCTION_ENTRY("rchcnt", SPU_INSTR_RR, INSTR_RCHCNT, instr_rchcnt), // 1e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 20000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 22000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 24000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 26000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 28000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 30000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 32000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 34000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 36000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 38000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3e000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 40000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 42000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 44000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 46000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 48000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 4a000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 4c000000
	INSTRUCTION_ENTRY("ori", SPU_INSTR_RI10, INSTR_ORI, instr_ori), // 4e000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 50000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 52000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 54000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 56000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 58000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 5a000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 5c000000
	INSTRUCTION_ENTRY("orhi", SPU_INSTR_RI10, INSTR_ORHI, instr_orhi), // 5e000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 60000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 62000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 64000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 66000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 68000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 6a000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 6c000000
	INSTRUCTION_ENTRY("orbi", SPU_INSTR_RI10, INSTR_ORBI, instr_orbi), // 6e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 70000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 72000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 74000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 76000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 78000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7e000000
	INSTRUCTION_ENTRY("sf", SPU_INSTR_RR, INSTR_SF, instr_sf), // 80000000
	INSTRUCTION_ENTRY("or", SPU_INSTR_RR, INSTR_OR, instr_or), // 82000000
	INSTRUCTION_ENTRY("bg", SPU_INSTR_RR, INSTR_BG, instr_bg), // 84000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 86000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 88000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 8a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 8c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 8e000000
	INSTRUCTION_ENTRY("sfh", SPU_INSTR_RR, INSTR_SFH, instr_sfh), // 90000000
	INSTRUCTION_ENTRY("nor", SPU_INSTR_RR, INSTR_NOR, instr_nor), // 92000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 94000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 96000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 98000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a4000000
	INSTRUCTION_ENTRY("absdb", SPU_INSTR_RR, INSTR_ABSDB, instr_absdb), // a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // b0000000
	INSTRUCTION_ENTRY("rotm", SPU_INSTR_RR, INSTR_ROTM, instr_rotm), // b2000000
	INSTRUCTION_ENTRY("rotma", SPU_INSTR_RR, INSTR_ROTMA, instr_rotma), // b4000000
	INSTRUCTION_ENTRY("shl", SPU_INSTR_RR, INSTR_SHL, instr_shl), // b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // be000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // c0000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // c2000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // c4000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // c6000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // c8000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // ca000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // cc000000
	INSTRUCTION_ENTRY("sfi", SPU_INSTR_RI10, INSTR_SFI, instr_sfi), // ce000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // d0000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // d2000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // d4000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // d6000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // d8000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // da000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // dc000000
	INSTRUCTION_ENTRY("sfhi", SPU_INSTR_RI10, INSTR_SFHI, instr_sfhi), // de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee000000
	INSTRUCTION_ENTRY("roti", SPU_INSTR_RI7, INSTR_ROTI, instr_roti), // f0000000
	INSTRUCTION_ENTRY("rotmi", SPU_INSTR_RI7, INSTR_ROTMI, instr_rotmi), // f2000000
	INSTRUCTION_ENTRY("rotmai", SPU_INSTR_RI7, INSTR_ROTMAI, instr_rotmai), // f4000000
	INSTRUCTION_ENTRY("shli", SPU_INSTR_RI7, INSTR_SHLI, instr_shli), // f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f8000000
	INSTRUCTION_ENTRY("rothmi", SPU_INSTR_RI7, INSTR_ROTHMI, instr_rothmi), // fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 100000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 102000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 104000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 106000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 108000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 10a000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 10c000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 10e000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 110000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 112000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 114000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 116000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 118000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 11a000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 11c000000
	INSTRUCTION_ENTRY("hbra", SPU_INSTR_RI18, INSTR_HBRA, instr_hbra), // 11e000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 120000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 122000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 124000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 126000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 128000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 12a000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 12c000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 12e000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 130000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 132000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 134000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 136000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 138000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 13a000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 13c000000
	INSTRUCTION_ENTRY("hbrr", SPU_INSTR_RI18, INSTR_HBRR, instr_hbrr), // 13e000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 140000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 142000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 144000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 146000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 148000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 14a000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 14c000000
	INSTRUCTION_ENTRY("andi", SPU_INSTR_RI10, INSTR_ANDI, instr_andi), // 14e000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 150000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 152000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 154000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 156000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 158000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 15a000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 15c000000
	INSTRUCTION_ENTRY("andhi", SPU_INSTR_RI10, INSTR_ANDHI, instr_andhi), // 15e000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 160000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 162000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 164000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 166000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 168000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 16a000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 16c000000
	INSTRUCTION_ENTRY("andbi", SPU_INSTR_RI10, INSTR_ANDBI, instr_andbi), // 16e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 170000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 172000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 174000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 176000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 178000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 17a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 17c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 17e000000
	INSTRUCTION_ENTRY("a", SPU_INSTR_RR, INSTR_A, instr_a), // 180000000
	INSTRUCTION_ENTRY("and", SPU_INSTR_RR, INSTR_AND, instr_and), // 182000000
	INSTRUCTION_ENTRY("cg", SPU_INSTR_RR, INSTR_CG, instr_cg), // 184000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 186000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 188000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 18a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 18c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 18e000000
	INSTRUCTION_ENTRY("ah", SPU_INSTR_RR, INSTR_AH, instr_ah), // 190000000
	INSTRUCTION_ENTRY("nand", SPU_INSTR_RR, INSTR_NAND, instr_nand), // 192000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 194000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 196000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 198000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 19a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 19c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 19e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1a4000000
	INSTRUCTION_ENTRY("avgb", SPU_INSTR_RR, INSTR_AVGB, instr_avgb), // 1a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1be000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1c0000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1c2000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1c4000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1c6000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1c8000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1ca000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1cc000000
	INSTRUCTION_ENTRY("ai", SPU_INSTR_RI10, INSTR_AI, instr_ai), // 1ce000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1d0000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1d2000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1d4000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1d6000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1d8000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1da000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1dc000000
	INSTRUCTION_ENTRY("ahi", SPU_INSTR_RI10, INSTR_AHI, instr_ahi), // 1de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 1fe000000
	INSTRUCTION_ENTRY("brz", SPU_INSTR_RI16, INSTR_BRZ, instr_brz), // 200000000
	INSTRUCTION_ENTRY("brz", SPU_INSTR_RI16, INSTR_BRZ, instr_brz), // 202000000
	INSTRUCTION_ENTRY("brz", SPU_INSTR_RI16, INSTR_BRZ, instr_brz), // 204000000
	INSTRUCTION_ENTRY("brz", SPU_INSTR_RI16, INSTR_BRZ, instr_brz), // 206000000
	INSTRUCTION_ENTRY("stqa", SPU_INSTR_RI16, INSTR_STQA, instr_stqa), // 208000000
	INSTRUCTION_ENTRY("stqa", SPU_INSTR_RI16, INSTR_STQA, instr_stqa), // 20a000000
	INSTRUCTION_ENTRY("stqa", SPU_INSTR_RI16, INSTR_STQA, instr_stqa), // 20c000000
	INSTRUCTION_ENTRY("stqa", SPU_INSTR_RI16, INSTR_STQA, instr_stqa), // 20e000000
	INSTRUCTION_ENTRY("brnz", SPU_INSTR_RI16, INSTR_BRNZ, instr_brnz), // 210000000
	INSTRUCTION_ENTRY("brnz", SPU_INSTR_RI16, INSTR_BRNZ, instr_brnz), // 212000000
	INSTRUCTION_ENTRY("brnz", SPU_INSTR_RI16, INSTR_BRNZ, instr_brnz), // 214000000
	INSTRUCTION_ENTRY("brnz", SPU_INSTR_RI16, INSTR_BRNZ, instr_brnz), // 216000000
	INSTRUCTION_ENTRY("mtspr", SPU_INSTR_RR, INSTR_MTSPR, instr_mtspr), // 218000000
	INSTRUCTION_ENTRY("wrch", SPU_INSTR_RR, INSTR_WRCH, instr_wrch), // 21a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 21c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 21e000000
	INSTRUCTION_ENTRY("brhz", SPU_INSTR_RI16, INSTR_BRHZ, instr_brhz), // 220000000
	INSTRUCTION_ENTRY("brhz", SPU_INSTR_RI16, INSTR_BRHZ, instr_brhz), // 222000000
	INSTRUCTION_ENTRY("brhz", SPU_INSTR_RI16, INSTR_BRHZ, instr_brhz), // 224000000
	INSTRUCTION_ENTRY("brhz", SPU_INSTR_RI16, INSTR_BRHZ, instr_brhz), // 226000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 228000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 22a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 22c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 22e000000
	INSTRUCTION_ENTRY("brhnz", SPU_INSTR_RI16, INSTR_BRHNZ, instr_brhnz), // 230000000
	INSTRUCTION_ENTRY("brhnz", SPU_INSTR_RI16, INSTR_BRHNZ, instr_brhnz), // 232000000
	INSTRUCTION_ENTRY("brhnz", SPU_INSTR_RI16, INSTR_BRHNZ, instr_brhnz), // 234000000
	INSTRUCTION_ENTRY("brhnz", SPU_INSTR_RI16, INSTR_BRHNZ, instr_brhnz), // 236000000
	INSTRUCTION_ENTRY("stqr", SPU_INSTR_RI16, INSTR_STQR, instr_stqr), // 238000000
	INSTRUCTION_ENTRY("stqr", SPU_INSTR_RI16, INSTR_STQR, instr_stqr), // 23a000000
	INSTRUCTION_ENTRY("stqr", SPU_INSTR_RI16, INSTR_STQR, instr_stqr), // 23c000000
	INSTRUCTION_ENTRY("stqr", SPU_INSTR_RI16, INSTR_STQR, instr_stqr), // 23e000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 240000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 242000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 244000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 246000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 248000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 24a000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 24c000000
	INSTRUCTION_ENTRY("stqd", SPU_INSTR_RI10, INSTR_STQD, instr_stqd), // 24e000000
	INSTRUCTION_ENTRY("biz", SPU_INSTR_RR, INSTR_BIZ, instr_biz), // 250000000
	INSTRUCTION_ENTRY("binz", SPU_INSTR_RR, INSTR_BINZ, instr_binz), // 252000000
	INSTRUCTION_ENTRY("bihz", SPU_INSTR_RR, INSTR_BIHZ, instr_bihz), // 254000000
	INSTRUCTION_ENTRY("bihnz", SPU_INSTR_RR, INSTR_BIHNZ, instr_bihnz), // 256000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 258000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 25a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 25c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 25e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 260000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 262000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 264000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 266000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 268000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 26a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 26c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 26e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 270000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 272000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 274000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 276000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 278000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 27a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 27c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 27e000000
	INSTRUCTION_ENTRY("stopd", SPU_INSTR_RR, INSTR_STOPD, instr_stopd), // 280000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 282000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 284000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 286000000
	INSTRUCTION_ENTRY("stqx", SPU_INSTR_RR, INSTR_STQX, instr_stqx), // 288000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 28a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 28c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 28e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 290000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 292000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 294000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 296000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 298000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 29a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 29c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 29e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2be000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2c8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2cc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2d0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2d2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2d4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2d6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2d8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2da000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2dc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 2fe000000
	INSTRUCTION_ENTRY("bra", SPU_INSTR_RI16, INSTR_BRA, instr_bra), // 300000000
	INSTRUCTION_ENTRY("bra", SPU_INSTR_RI16, INSTR_BRA, instr_bra), // 302000000
	INSTRUCTION_ENTRY("bra", SPU_INSTR_RI16, INSTR_BRA, instr_bra), // 304000000
	INSTRUCTION_ENTRY("bra", SPU_INSTR_RI16, INSTR_BRA, instr_bra), // 306000000
	INSTRUCTION_ENTRY("lqa", SPU_INSTR_RI16, INSTR_LQA, instr_lqa), // 308000000
	INSTRUCTION_ENTRY("lqa", SPU_INSTR_RI16, INSTR_LQA, instr_lqa), // 30a000000
	INSTRUCTION_ENTRY("lqa", SPU_INSTR_RI16, INSTR_LQA, instr_lqa), // 30c000000
	INSTRUCTION_ENTRY("lqa", SPU_INSTR_RI16, INSTR_LQA, instr_lqa), // 30e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 310000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 312000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 314000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 316000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 318000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 31a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 31c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 31e000000
	INSTRUCTION_ENTRY("br", SPU_INSTR_RI16, INSTR_BR, instr_br), // 320000000
	INSTRUCTION_ENTRY("br", SPU_INSTR_RI16, INSTR_BR, instr_br), // 322000000
	INSTRUCTION_ENTRY("br", SPU_INSTR_RI16, INSTR_BR, instr_br), // 324000000
	INSTRUCTION_ENTRY("br", SPU_INSTR_RI16, INSTR_BR, instr_br), // 326000000
	INSTRUCTION_ENTRY("fsmbi", SPU_INSTR_RI16, INSTR_FSMBI, instr_fsmbi), // 328000000
	INSTRUCTION_ENTRY("fsmbi", SPU_INSTR_RI16, INSTR_FSMBI, instr_fsmbi), // 32a000000
	INSTRUCTION_ENTRY("fsmbi", SPU_INSTR_RI16, INSTR_FSMBI, instr_fsmbi), // 32c000000
	INSTRUCTION_ENTRY("fsmbi", SPU_INSTR_RI16, INSTR_FSMBI, instr_fsmbi), // 32e000000
	INSTRUCTION_ENTRY("brsl", SPU_INSTR_RI16, INSTR_BRSL, instr_brsl), // 330000000
	INSTRUCTION_ENTRY("brsl", SPU_INSTR_RI16, INSTR_BRSL, instr_brsl), // 332000000
	INSTRUCTION_ENTRY("brsl", SPU_INSTR_RI16, INSTR_BRSL, instr_brsl), // 334000000
	INSTRUCTION_ENTRY("brsl", SPU_INSTR_RI16, INSTR_BRSL, instr_brsl), // 336000000
	INSTRUCTION_ENTRY("lqr", SPU_INSTR_RI16, INSTR_LQR, instr_lqr), // 338000000
	INSTRUCTION_ENTRY("lqr", SPU_INSTR_RI16, INSTR_LQR, instr_lqr), // 33a000000
	INSTRUCTION_ENTRY("lqr", SPU_INSTR_RI16, INSTR_LQR, instr_lqr), // 33c000000
	INSTRUCTION_ENTRY("lqr", SPU_INSTR_RI16, INSTR_LQR, instr_lqr), // 33e000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 340000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 342000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 344000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 346000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 348000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 34a000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 34c000000
	INSTRUCTION_ENTRY("lqd", SPU_INSTR_RI10, INSTR_LQD, instr_lqd), // 34e000000
	INSTRUCTION_ENTRY("bi", SPU_INSTR_RR, INSTR_BI, instr_bi), // 350000000
	INSTRUCTION_ENTRY("bisl", SPU_INSTR_RR, INSTR_BISL, instr_bisl), // 352000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 354000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 356000000
	INSTRUCTION_ENTRY("hbr", SPU_INSTR_SPECIAL, INSTR_HBR, instr_hbr), // 358000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 35a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 35c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 35e000000
	INSTRUCTION_ENTRY("gb", SPU_INSTR_RR, INSTR_GB, instr_gb), // 360000000
	INSTRUCTION_ENTRY("gbh", SPU_INSTR_RR, INSTR_GBH, instr_gbh), // 362000000
	INSTRUCTION_ENTRY("gbb", SPU_INSTR_RR, INSTR_GBB, instr_gbb), // 364000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 366000000
	INSTRUCTION_ENTRY("fsm", SPU_INSTR_RR, INSTR_FSM, instr_fsm), // 368000000
	INSTRUCTION_ENTRY("fsmh", SPU_INSTR_RR, INSTR_FSMH, instr_fsmh), // 36a000000
	INSTRUCTION_ENTRY("fsmb", SPU_INSTR_RR, INSTR_FSMB, instr_fsmb), // 36c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 36e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 370000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 372000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 374000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 376000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 378000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 37a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 37c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 37e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 380000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 382000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 384000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 386000000
	INSTRUCTION_ENTRY("lqx", SPU_INSTR_RR, INSTR_LQX, instr_lqx), // 388000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 38a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 38c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 38e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 390000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 392000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 394000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 396000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 398000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 39a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 39c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 39e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3a6000000
	INSTRUCTION_ENTRY("cbx", SPU_INSTR_RR, INSTR_CBX, instr_cbx), // 3a8000000
	INSTRUCTION_ENTRY("chx", SPU_INSTR_RR, INSTR_CHX, instr_chx), // 3aa000000
	INSTRUCTION_ENTRY("cwx", SPU_INSTR_RR, INSTR_CWX, instr_cwx), // 3ac000000
	INSTRUCTION_ENTRY("cdx", SPU_INSTR_RR, INSTR_CDX, instr_cdx), // 3ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3b0000000
	INSTRUCTION_ENTRY("rotqmbi", SPU_INSTR_RR, INSTR_ROTQMBI, instr_rotqmbi), // 3b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3b4000000
	INSTRUCTION_ENTRY("shlqbi", SPU_INSTR_RR, INSTR_SHLQBI, instr_shlqbi), // 3b6000000
	INSTRUCTION_ENTRY("rotqby", SPU_INSTR_RR, INSTR_ROTQBY, instr_rotqby), // 3b8000000
	INSTRUCTION_ENTRY("rotqmby", SPU_INSTR_RR, INSTR_ROTQMBY, instr_rotqmby), // 3ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3bc000000
	INSTRUCTION_ENTRY("shlqby", SPU_INSTR_RR, INSTR_SHLQBY, instr_shlqby), // 3be000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3c8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3ca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3cc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3ce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3d0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3d2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3d4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3d6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3d8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3da000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3dc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3de000000
	INSTRUCTION_ENTRY("orx", SPU_INSTR_RR, INSTR_ORX, instr_orx), // 3e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3e6000000
	INSTRUCTION_ENTRY("cbd", SPU_INSTR_RI7, INSTR_CBD, instr_cbd), // 3e8000000
	INSTRUCTION_ENTRY("chd", SPU_INSTR_RI7, INSTR_CHD, instr_chd), // 3ea000000
	INSTRUCTION_ENTRY("cwd", SPU_INSTR_RI7, INSTR_CWD, instr_cwd), // 3ec000000
	INSTRUCTION_ENTRY("cdd", SPU_INSTR_RI7, INSTR_CDD, instr_cdd), // 3ee000000
	INSTRUCTION_ENTRY("rotqbii", SPU_INSTR_RI7, INSTR_ROTQBII, instr_rotqbii), // 3f0000000
	INSTRUCTION_ENTRY("rotqmbii", SPU_INSTR_RI7, INSTR_ROTQMBII, instr_rotqmbii), // 3f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3f4000000
	INSTRUCTION_ENTRY("shlqbii", SPU_INSTR_RI7, INSTR_SHLQBII, instr_shlqbii), // 3f6000000
	INSTRUCTION_ENTRY("rotqbyi", SPU_INSTR_RI7, INSTR_ROTQBYI, instr_rotqbyi), // 3f8000000
	INSTRUCTION_ENTRY("rotqmbyi", SPU_INSTR_RI7, INSTR_ROTQMBYI, instr_rotqmbyi), // 3fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 3fc000000
	INSTRUCTION_ENTRY("shlqbyi", SPU_INSTR_RI7, INSTR_SHLQBYI, instr_shlqbyi), // 3fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 400000000
	INSTRUCTION_ENTRY("nop", SPU_INSTR_SPECIAL, INSTR_NOP, instr_nop), // 402000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 404000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 406000000
	INSTRUCTION_ENTRY("il", SPU_INSTR_RI16, INSTR_IL, instr_il), // 408000000
	INSTRUCTION_ENTRY("il", SPU_INSTR_RI16, INSTR_IL, instr_il), // 40a000000
	INSTRUCTION_ENTRY("il", SPU_INSTR_RI16, INSTR_IL, instr_il), // 40c000000
	INSTRUCTION_ENTRY("il", SPU_INSTR_RI16, INSTR_IL, instr_il), // 40e000000
	INSTRUCTION_ENTRY("ilhu", SPU_INSTR_RI16, INSTR_ILHU, instr_ilhu), // 410000000
	INSTRUCTION_ENTRY("ilhu", SPU_INSTR_RI16, INSTR_ILHU, instr_ilhu), // 412000000
	INSTRUCTION_ENTRY("ilhu", SPU_INSTR_RI16, INSTR_ILHU, instr_ilhu), // 414000000
	INSTRUCTION_ENTRY("ilhu", SPU_INSTR_RI16, INSTR_ILHU, instr_ilhu), // 416000000
	INSTRUCTION_ENTRY("ilh", SPU_INSTR_RI16, INSTR_ILH, instr_ilh), // 418000000
	INSTRUCTION_ENTRY("ilh", SPU_INSTR_RI16, INSTR_ILH, instr_ilh), // 41a000000
	INSTRUCTION_ENTRY("ilh", SPU_INSTR_RI16, INSTR_ILH, instr_ilh), // 41c000000
	INSTRUCTION_ENTRY("ilh", SPU_INSTR_RI16, INSTR_ILH, instr_ilh), // 41e000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 420000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 422000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 424000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 426000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 428000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 42a000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 42c000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 42e000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 430000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 432000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 434000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 436000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 438000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 43a000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 43c000000
	INSTRUCTION_ENTRY("ila", SPU_INSTR_RI18, INSTR_ILA, instr_ila), // 43e000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 440000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 442000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 444000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 446000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 448000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 44a000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 44c000000
	INSTRUCTION_ENTRY("xori", SPU_INSTR_RI10, INSTR_XORI, instr_xori), // 44e000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 450000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 452000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 454000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 456000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 458000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 45a000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 45c000000
	INSTRUCTION_ENTRY("xorhi", SPU_INSTR_RI10, INSTR_XORHI, instr_xorhi), // 45e000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 460000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 462000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 464000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 466000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 468000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 46a000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 46c000000
	INSTRUCTION_ENTRY("xorbi", SPU_INSTR_RI10, INSTR_XORBI, instr_xorbi), // 46e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 470000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 472000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 474000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 476000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 478000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 47a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 47c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 47e000000
	INSTRUCTION_ENTRY("cgt", SPU_INSTR_RR, INSTR_CGT, instr_cgt), // 480000000
	INSTRUCTION_ENTRY("xor", SPU_INSTR_RR, INSTR_XOR, instr_xor), // 482000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 484000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 486000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 488000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 48a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 48c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 48e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 490000000
	INSTRUCTION_ENTRY("eqv", SPU_INSTR_RR, INSTR_EQV, instr_eqv), // 492000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 494000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 496000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 498000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 49a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 49c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 49e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4a4000000
	INSTRUCTION_ENTRY("sumb", SPU_INSTR_RR, INSTR_SUMB, instr_sumb), // 4a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4be000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4c0000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4c2000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4c4000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4c6000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4c8000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4ca000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4cc000000
	INSTRUCTION_ENTRY("cgti", SPU_INSTR_RI10, INSTR_CGTI, instr_cgti), // 4ce000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4d0000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4d2000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4d4000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4d6000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4d8000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4da000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4dc000000
	INSTRUCTION_ENTRY("cgthi", SPU_INSTR_RI10, INSTR_CGTHI, instr_cgthi), // 4de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 4fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 500000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 502000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 504000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 506000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 508000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 50a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 50c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 50e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 510000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 512000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 514000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 516000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 518000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 51a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 51c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 51e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 520000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 522000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 524000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 526000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 528000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 52a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 52c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 52e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 530000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 532000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 534000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 536000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 538000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 53a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 53c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 53e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 540000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 542000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 544000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 546000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 548000000
	INSTRUCTION_ENTRY("clz", SPU_INSTR_RR, INSTR_CLZ, instr_clz), // 54a000000
	INSTRUCTION_ENTRY("xswd", SPU_INSTR_RR, INSTR_XSWD, instr_xswd), // 54c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 54e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 550000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 552000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 554000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 556000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 558000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 55a000000
	INSTRUCTION_ENTRY("xshw", SPU_INSTR_RR, INSTR_XSHW, instr_xshw), // 55c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 55e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 560000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 562000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 564000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 566000000
	INSTRUCTION_ENTRY("cntb", SPU_INSTR_RR, INSTR_CNTB, instr_cntb), // 568000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 56a000000
	INSTRUCTION_ENTRY("xsbh", SPU_INSTR_RR, INSTR_XSBH, instr_xsbh), // 56c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 56e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 570000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 572000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 574000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 576000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 578000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 57a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 57c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 57e000000
	INSTRUCTION_ENTRY("clgt", SPU_INSTR_RR, INSTR_CLGT, instr_clgt), // 580000000
	INSTRUCTION_ENTRY("andc", SPU_INSTR_RR, INSTR_ANDC, instr_andc), // 582000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 584000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 586000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 588000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 58a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 58c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 58e000000
	INSTRUCTION_ENTRY("clgth", SPU_INSTR_RR, INSTR_CLGTH, instr_clgth), // 590000000
	INSTRUCTION_ENTRY("orc", SPU_INSTR_RR, INSTR_ORC, instr_orc), // 592000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 594000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 596000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 598000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 59a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 59c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 59e000000
	INSTRUCTION_ENTRY("clgtb", SPU_INSTR_RR, INSTR_CLGTB, instr_clgtb), // 5a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5be000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5c0000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5c2000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5c4000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5c6000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5c8000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5ca000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5cc000000
	INSTRUCTION_ENTRY("clgti", SPU_INSTR_RI10, INSTR_CLGTI, instr_clgti), // 5ce000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5d0000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5d2000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5d4000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5d6000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5d8000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5da000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5dc000000
	INSTRUCTION_ENTRY("clgthi", SPU_INSTR_RI10, INSTR_CLGTHI, instr_clgthi), // 5de000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5e0000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5e2000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5e4000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5e6000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5e8000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5ea000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5ec000000
	INSTRUCTION_ENTRY("clgtbi", SPU_INSTR_RI10, INSTR_CLGTBI, instr_clgtbi), // 5ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 5fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 600000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 602000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 604000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 606000000
	INSTRUCTION_ENTRY("iohl", SPU_INSTR_RI16, INSTR_IOHL, instr_iohl), // 608000000
	INSTRUCTION_ENTRY("iohl", SPU_INSTR_RI16, INSTR_IOHL, instr_iohl), // 60a000000
	INSTRUCTION_ENTRY("iohl", SPU_INSTR_RI16, INSTR_IOHL, instr_iohl), // 60c000000
	INSTRUCTION_ENTRY("iohl", SPU_INSTR_RI16, INSTR_IOHL, instr_iohl), // 60e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 610000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 612000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 614000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 616000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 618000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 61a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 61c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 61e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 620000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 622000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 624000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 626000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 628000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 62a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 62c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 62e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 630000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 632000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 634000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 636000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 638000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 63a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 63c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 63e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 640000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 642000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 644000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 646000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 648000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 64a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 64c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 64e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 650000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 652000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 654000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 656000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 658000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 65a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 65c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 65e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 660000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 662000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 664000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 666000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 668000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 66a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 66c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 66e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 670000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 672000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 674000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 676000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 678000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 67a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 67c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 67e000000
	INSTRUCTION_ENTRY("addx", SPU_INSTR_RR, INSTR_ADDX, instr_addx), // 680000000
	INSTRUCTION_ENTRY("sfx", SPU_INSTR_RR, INSTR_SFX, instr_sfx), // 682000000
	INSTRUCTION_ENTRY("cgx", SPU_INSTR_RR, INSTR_CGX, instr_cgx), // 684000000
	INSTRUCTION_ENTRY("bgx", SPU_INSTR_RR, INSTR_BGX, instr_bgx), // 686000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 688000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 68a000000
	INSTRUCTION_ENTRY("mpyhha", SPU_INSTR_RR, INSTR_MPYHHA, instr_mpyhha), // 68c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 68e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 690000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 692000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 694000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 696000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 698000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 69a000000
	INSTRUCTION_ENTRY("mpyhhau", SPU_INSTR_RR, INSTR_MPYHHAU, instr_mpyhhau), // 69c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 69e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6be000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6c0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6c2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6c4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6c6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6c8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6cc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6d0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6d2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6d4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6d6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6d8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6da000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6dc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 6fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 700000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 702000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 704000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 706000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 708000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 70a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 70c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 70e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 710000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 712000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 714000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 716000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 718000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 71a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 71c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 71e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 720000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 722000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 724000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 726000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 728000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 72a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 72c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 72e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 730000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 732000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 734000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 736000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 738000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 73a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 73c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 73e000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 740000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 742000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 744000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 746000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 748000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 74a000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 74c000000
	INSTRUCTION_ENTRY("mpyi", SPU_INSTR_RI10, INSTR_MPYI, instr_mpyi), // 74e000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 750000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 752000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 754000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 756000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 758000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 75a000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 75c000000
	INSTRUCTION_ENTRY("mpyui", SPU_INSTR_RI10, INSTR_MPYUI, instr_mpyui), // 75e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 760000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 762000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 764000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 766000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 768000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 76a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 76c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 76e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 770000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 772000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 774000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 776000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 778000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 77a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 77c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 77e000000
	INSTRUCTION_ENTRY("ceq", SPU_INSTR_RR, INSTR_CEQ, instr_ceq), // 780000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 782000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 784000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 786000000
	INSTRUCTION_ENTRY("mpy", SPU_INSTR_RR, INSTR_MPY, instr_mpy), // 788000000
	INSTRUCTION_ENTRY("mpyh", SPU_INSTR_RR, INSTR_MPYH, instr_mpyh), // 78a000000
	INSTRUCTION_ENTRY("mpyhh", SPU_INSTR_RR, INSTR_MPYHH, instr_mpyhh), // 78c000000
	INSTRUCTION_ENTRY("mpys", SPU_INSTR_RR, INSTR_MPYS, instr_mpys), // 78e000000
	INSTRUCTION_ENTRY("ceqh", SPU_INSTR_RR, INSTR_CEQH, instr_ceqh), // 790000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 792000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 794000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 796000000
	INSTRUCTION_ENTRY("mpyu", SPU_INSTR_RR, INSTR_MPYU, instr_mpyu), // 798000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 79a000000
	INSTRUCTION_ENTRY("mpyhhu", SPU_INSTR_RR, INSTR_MPYHHU, instr_mpyhhu), // 79c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 79e000000
	INSTRUCTION_ENTRY("ceqb", SPU_INSTR_RR, INSTR_CEQB, instr_ceqb), // 7a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 7be000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7c0000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7c2000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7c4000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7c6000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7c8000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7ca000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7cc000000
	INSTRUCTION_ENTRY("ceqi", SPU_INSTR_RI10, INSTR_CEQI, instr_ceqi), // 7ce000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7d0000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7d2000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7d4000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7d6000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7d8000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7da000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7dc000000
	INSTRUCTION_ENTRY("ceqhi", SPU_INSTR_RI10, INSTR_CEQHI, instr_ceqhi), // 7de000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7e0000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7e2000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7e4000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7e6000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7e8000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7ea000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7ec000000
	INSTRUCTION_ENTRY("ceqbi", SPU_INSTR_RI10, INSTR_CEQBI, instr_ceqbi), // 7ee000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7f0000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7f2000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7f4000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7f6000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7f8000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7fa000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7fc000000
	INSTRUCTION_ENTRY("heqi", SPU_INSTR_RI10, INSTR_HEQI, instr_heqi), // 7fe000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 800000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 802000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 804000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 806000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 808000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 80a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 80c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 80e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 810000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 812000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 814000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 816000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 818000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 81a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 81c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 81e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 820000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 822000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 824000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 826000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 828000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 82a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 82c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 82e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 830000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 832000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 834000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 836000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 838000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 83a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 83c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 83e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 840000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 842000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 844000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 846000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 848000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 84a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 84c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 84e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 850000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 852000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 854000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 856000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 858000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 85a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 85c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 85e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 860000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 862000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 864000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 866000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 868000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 86a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 86c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 86e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 870000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 872000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 874000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 876000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 878000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 87a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 87c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 87e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 880000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 882000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 884000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 886000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 888000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 88a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 88c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 88e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 890000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 892000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 894000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 896000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 898000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 89a000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 89c000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 89e000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8a0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8a2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8a4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8a6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8a8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8aa000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ac000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ae000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8b0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8b2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8b4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8b6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8b8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ba000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8bc000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8be000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8c0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8c2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8c4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8c6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8c8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ca000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8cc000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ce000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8d0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8d2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8d4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8d6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8d8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8da000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8dc000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8de000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8e0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8e2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8e4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8e6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8e8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ea000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ec000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8ee000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8f0000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8f2000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8f4000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8f6000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8f8000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8fa000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8fc000000
	INSTRUCTION_ENTRY("selb", SPU_INSTR_RRR, INSTR_SELB, instr_selb), // 8fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 900000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 902000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 904000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 906000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 908000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 90a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 90c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 90e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 910000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 912000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 914000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 916000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 918000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 91a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 91c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 91e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 920000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 922000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 924000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 926000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 928000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 92a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 92c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 92e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 930000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 932000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 934000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 936000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 938000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 93a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 93c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 93e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 940000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 942000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 944000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 946000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 948000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 94a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 94c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 94e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 950000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 952000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 954000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 956000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 958000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 95a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 95c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 95e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 960000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 962000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 964000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 966000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 968000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 96a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 96c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 96e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 970000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 972000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 974000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 976000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 978000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 97a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 97c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 97e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 980000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 982000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 984000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 986000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 988000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 98a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 98c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 98e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 990000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 992000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 994000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 996000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 998000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 99a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 99c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 99e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9a8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9aa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9b0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9b2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9b4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9b6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9b8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9bc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9be000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9c8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9cc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9d0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9d2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9d4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9d6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9d8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9da000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9dc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9de000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9e8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9ee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9f0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9f2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9f4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9f6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9f8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9fa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9fc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // 9fe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a00000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a02000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a04000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a06000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a08000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a0a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a0c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a0e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a10000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a12000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a14000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a16000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a18000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a1a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a1c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a1e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a20000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a22000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a24000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a26000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a28000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a2a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a2c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a2e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a30000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a32000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a34000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a36000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a38000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a3a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a3c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a3e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a40000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a42000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a44000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a46000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a48000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a4a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a4c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a4e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a50000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a52000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a54000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a56000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a58000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a5a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a5c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a5e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a60000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a62000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a64000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a66000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a68000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a6a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a6c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a6e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a70000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a72000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a74000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a76000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a78000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a7a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a7c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a7e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a80000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a82000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a84000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a86000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a88000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a8a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a8c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a8e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a90000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a92000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a94000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a96000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a98000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a9a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a9c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // a9e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aa8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aaa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ab0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ab2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ab4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ab6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ab8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // abc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // abe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ac8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // acc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ace000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ad0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ad2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ad4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ad6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ad8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ada000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // adc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ade000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ae8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // aee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // af0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // af2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // af4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // af6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // af8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // afa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // afc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // afe000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b00000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b02000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b04000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b06000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b08000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b0a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b0c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b0e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b10000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b12000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b14000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b16000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b18000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b1a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b1c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b1e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b20000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b22000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b24000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b26000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b28000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b2a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b2c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b2e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b30000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b32000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b34000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b36000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b38000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b3a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b3c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b3e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b40000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b42000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b44000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b46000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b48000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b4a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b4c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b4e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b50000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b52000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b54000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b56000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b58000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b5a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b5c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b5e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b60000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b62000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b64000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b66000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b68000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b6a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b6c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b6e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b70000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b72000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b74000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b76000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b78000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b7a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b7c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b7e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b80000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b82000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b84000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b86000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b88000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b8a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b8c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b8e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b90000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b92000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b94000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b96000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b98000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b9a000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b9c000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // b9e000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // ba0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // ba2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // ba4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // ba6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // ba8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // baa000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bac000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bae000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bb0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bb2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bb4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bb6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bb8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bba000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bbc000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bbe000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bc0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bc2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bc4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bc6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bc8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bca000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bcc000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bce000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bd0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bd2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bd4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bd6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bd8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bda000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bdc000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bde000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // be0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // be2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // be4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // be6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // be8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bea000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bec000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bee000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bf0000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bf2000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bf4000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bf6000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bf8000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bfa000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bfc000000
	INSTRUCTION_ENTRY("shufb", SPU_INSTR_RRR, INSTR_SHUFB, instr_shufb), // bfe000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c00000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c02000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c04000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c06000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c08000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c0a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c0c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c0e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c10000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c12000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c14000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c16000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c18000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c1a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c1c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c1e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c20000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c22000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c24000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c26000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c28000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c2a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c2c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c2e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c30000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c32000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c34000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c36000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c38000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c3a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c3c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c3e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c40000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c42000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c44000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c46000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c48000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c4a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c4c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c4e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c50000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c52000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c54000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c56000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c58000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c5a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c5c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c5e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c60000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c62000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c64000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c66000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c68000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c6a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c6c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c6e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c70000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c72000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c74000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c76000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c78000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c7a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c7c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c7e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c80000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c82000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c84000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c86000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c88000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c8a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c8c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c8e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c90000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c92000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c94000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c96000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c98000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c9a000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c9c000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // c9e000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ca0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ca2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ca4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ca6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ca8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // caa000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cac000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cae000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cb0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cb2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cb4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cb6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cb8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cba000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cbc000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cbe000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cc0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cc2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cc4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cc6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cc8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cca000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ccc000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cce000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cd0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cd2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cd4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cd6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cd8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cda000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cdc000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cde000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ce0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ce2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ce4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ce6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // ce8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cea000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cec000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cee000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cf0000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cf2000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cf4000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cf6000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cf8000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cfa000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cfc000000
	INSTRUCTION_ENTRY("mpya", SPU_INSTR_RRR, INSTR_MPYA, instr_mpya), // cfe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d00000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d02000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d04000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d06000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d08000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d0a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d0c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d0e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d10000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d12000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d14000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d16000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d18000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d1a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d1c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d1e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d20000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d22000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d24000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d26000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d28000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d2a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d2c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d2e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d30000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d32000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d34000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d36000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d38000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d3a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d3c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d3e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d40000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d42000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d44000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d46000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d48000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d4a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d4c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d4e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d50000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d52000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d54000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d56000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d58000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d5a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d5c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d5e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d60000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d62000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d64000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d66000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d68000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d6a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d6c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d6e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d70000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d72000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d74000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d76000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d78000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d7a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d7c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d7e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d80000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d82000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d84000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d86000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d88000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d8a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d8c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d8e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d90000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d92000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d94000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d96000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d98000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d9a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d9c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // d9e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // da0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // da2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // da4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // da6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // da8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // daa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // db0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // db2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // db4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // db6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // db8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dbc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dbe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dc0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dc2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dc4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dc6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dc8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dcc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dd0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dd2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dd4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dd6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dd8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dda000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ddc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dde000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // de0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // de2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // de4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // de6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // de8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // df0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // df2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // df4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // df6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // df8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dfa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dfc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // dfe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e00000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e02000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e04000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e06000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e08000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e0a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e0c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e0e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e10000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e12000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e14000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e16000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e18000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e1a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e1c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e1e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e20000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e22000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e24000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e26000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e28000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e2a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e2c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e2e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e30000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e32000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e34000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e36000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e38000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e3a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e3c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e3e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e40000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e42000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e44000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e46000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e48000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e4a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e4c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e4e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e50000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e52000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e54000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e56000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e58000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e5a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e5c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e5e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e60000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e62000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e64000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e66000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e68000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e6a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e6c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e6e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e70000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e72000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e74000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e76000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e78000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e7a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e7c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e7e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e80000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e82000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e84000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e86000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e88000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e8a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e8c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e8e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e90000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e92000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e94000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e96000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e98000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e9a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e9c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // e9e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ea8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eaa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eb0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eb2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eb4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eb6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eb8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ebc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ebe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ec8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ecc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ece000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ed0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ed2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ed4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ed6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ed8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eda000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // edc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ede000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ee8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // eee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ef0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ef2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ef4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ef6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ef8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // efa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // efc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // efe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f00000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f02000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f04000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f06000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f08000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f0a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f0c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f0e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f10000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f12000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f14000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f16000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f18000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f1a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f1c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f1e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f20000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f22000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f24000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f26000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f28000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f2a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f2c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f2e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f30000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f32000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f34000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f36000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f38000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f3a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f3c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f3e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f40000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f42000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f44000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f46000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f48000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f4a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f4c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f4e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f50000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f52000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f54000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f56000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f58000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f5a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f5c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f5e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f60000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f62000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f64000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f66000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f68000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f6a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f6c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f6e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f70000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f72000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f74000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f76000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f78000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f7a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f7c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f7e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f80000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f82000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f84000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f86000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f88000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f8a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f8c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f8e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f90000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f92000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f94000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f96000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f98000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f9a000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f9c000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // f9e000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fa0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fa2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fa4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fa6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fa8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // faa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fac000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fae000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fb0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fb2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fb4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fb6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fb8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fba000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fbc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fbe000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fc8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fca000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fcc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fce000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fd0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fd2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fd4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fd6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fd8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fda000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fdc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fde000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fe8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fea000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fec000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // fee000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ff0000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ff2000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ff4000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ff6000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ff8000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ffa000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ffc000000
	INSTRUCTION_ENTRY("INSTR_NONE", SPU_INSTR_NONE, INSTR_NONE, NULL), // ffe000000
};

typedef instr_t (*spu_instr_rr_t)(u32 rt, u32 ra, u32 rb);
typedef instr_t (*spu_instr_rrr_t)(u32 rt, u32 ra, u32 rb, u32 rc);
typedef instr_t (*spu_instr_ri7_t)(u32 rt, u32 ra, u32 i7);
typedef instr_t (*spu_instr_ri10_t)(u32 rt, u32 ra, u32 i10);
typedef instr_t (*spu_instr_ri16_t)(u32 rt, u32 i16);
typedef instr_t (*spu_instr_ri18_t)(u32 rt, u32 i18);
typedef instr_t (*spu_instr_special_t)(u32 opcode);

#define INSTR_BITS(instr, start, end) (instr >> (31 - end)) & ((1 << (end - start + 1)) - 1)

instr_t disasm_disassemble_instr(u32 instr)
{
	instr_t res;
	u32 rt, ra, rb, rc, ix, op = INSTR_BITS(instr, 0, 10);

	//Set parameters.
	switch(instr_tbl[op].type)
	{
	case SPU_INSTR_RR:
		rb = INSTR_BITS(instr, 11, 17);
		ra = INSTR_BITS(instr, 18, 24);
		rt = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_rr_t)instr_tbl[op].ptr)(rt, ra, rb);
		break;
	case SPU_INSTR_RRR:
		rt = INSTR_BITS(instr, 4, 10);
		rb = INSTR_BITS(instr, 11, 17);
		ra = INSTR_BITS(instr, 18, 24);
		rc = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_rrr_t)instr_tbl[op].ptr)(rt, ra, rb, rc);
		break;
	case SPU_INSTR_RI7:
		ix = INSTR_BITS(instr, 11, 17);
		ra = INSTR_BITS(instr, 18, 24);
		rt = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_ri7_t)instr_tbl[op].ptr)(rt, ra, ix);
		break;
	case SPU_INSTR_RI10:
		ix = INSTR_BITS(instr, 8, 17);
		ra = INSTR_BITS(instr, 18, 24);
		rt = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_ri10_t)instr_tbl[op].ptr)(rt, ra, ix);
		break;
	case SPU_INSTR_RI16:
		ix = INSTR_BITS(instr, 9, 24);
		rt = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_ri16_t)instr_tbl[op].ptr)(rt, ix);
		break;
	case SPU_INSTR_RI18:
		ix = INSTR_BITS(instr, 7, 24);
		rt = INSTR_BITS(instr, 25, 31);
		res = ((spu_instr_ri18_t)instr_tbl[op].ptr)(rt, ix);
		break;
	case SPU_INSTR_SPECIAL:
		res = ((spu_instr_special_t)instr_tbl[op].ptr)(instr);
	}

	//Set name.
	res.name = instr_tbl[op].name;
	//Set instruction.
	res.instr = instr_tbl[op].instr;
	//Set type.
	res.type = instr_tbl[op].type;
	//Set opcode.
	res.opcode = instr;

	return res;
}

void disasm_print_imm(char* fp, s32 imm)
{
    if(imm < 0)
        sprintf(fp, "-0x%x", (u32)(-imm));
    else
        sprintf(fp, "0x%x", imm);
}

std::string disasm_print_instr(instr_t *inst, bool print_addr)
{
    char buf[100];
    auto fp = buf;
    if(print_addr)
        fp += sprintf(fp, "0x%05x %-7s", inst->address, inst->name);
    else
        fp += sprintf(fp, "%-7s", inst->name);

    switch(inst->type)
    {
    case SPU_INSTR_RR:
        sprintf(fp, " $%d, $%d, $%d", inst->rr.rt, inst->rr.ra, inst->rr.rb);
        break;
    case SPU_INSTR_RRR:
        sprintf(fp, " $%d, $%d, $%d, $%d", inst->rrr.rt, inst->rrr.ra, inst->rrr.rb, inst->rrr.rc);
        break;
    case SPU_INSTR_RI7:
        fp += sprintf(fp, " $%d, $%d, ", inst->ri7.rt, inst->ri7.ra);
        disasm_print_imm(fp, inst->ri7.i7);
        break;
    case SPU_INSTR_RI10:
        fp += sprintf(fp, " $%d, $%d, ", inst->ri10.rt, inst->ri10.ra);
        disasm_print_imm(fp, inst->ri10.i10);
        break;
    case SPU_INSTR_RI16:
        if(inst->name[0] == 'b' && inst->name[1] == 'r')
            sprintf(fp, " $%d, 0x%x", inst->ri16.rt, inst->address + inst->ri16.i16);
        else if(inst->instr == INSTR_LQR)
            sprintf(fp, " $%d, 0x%x", inst->ri16.rt, inst->address + inst->ri16.i16);
        else if(inst->instr == INSTR_STQR)
            sprintf(fp, " $%d, 0x%x", inst->ri16.rt, inst->address + inst->ri16.i16);
        else
            sprintf(fp, " $%d, 0x%x", inst->ri16.rt, inst->ri16.i16);
        break;
    case SPU_INSTR_RI18:
        fp += sprintf(fp, " $%d, ", inst->ri18.rt);
        disasm_print_imm(fp, inst->ri18.i18);
        break;
    }
    return std::string(buf);
}

bool disasm_is_branch(instr_t *inst)
{
	static u32 branch_instrs[13] = 
	{
		INSTR_BI,
		INSTR_BIHNZ,
		INSTR_BIHZ,
		INSTR_BINZ,
		INSTR_BISL,
		INSTR_BIZ,
		INSTR_BR,
		INSTR_BRA,
		INSTR_BRHNZ,
		INSTR_BRHZ,
		INSTR_BRNZ,
		INSTR_BRSL,
		INSTR_BRZ
	};

	int i;

	for(i = 0; i < 13; i++)
		if(static_cast<uint32_t>(inst->instr) == branch_instrs[i])
			return true;
	return false;
}

#define BRCNT 7
bool disasm_is_direct_branch(instr_t *inst)
{
	static u32 branch_instrs[BRCNT] = 
	{
		INSTR_BR,
		INSTR_BRA,
		INSTR_BRHNZ,
		INSTR_BRHZ,
		INSTR_BRNZ,
		INSTR_BRSL,
		INSTR_BRZ
	};

	int i;

	for(i = 0; i < BRCNT; i++)
		if(static_cast<uint32_t>(inst->instr) == branch_instrs[i])
			return true;
	return false;
}
#undef BRCNT

#define BRCNT 4
bool disasm_is_direct_cond_branch(instr_t *inst)
{
	static u32 branch_instrs[BRCNT] = 
	{
		INSTR_BRHNZ,
		INSTR_BRHZ,
		INSTR_BRNZ,
		INSTR_BRZ
	};

	int i;

	for(i = 0; i < BRCNT; i++)
            if(static_cast<uint32_t>(inst->instr) == branch_instrs[i])
			return true;
	return false;
}
#undef BRCNT

#define BRCNT 3
bool disasm_is_direct_uncond_branch(instr_t *inst)
{
	static u32 branch_instrs[BRCNT] = 
	{
		INSTR_BR,
		INSTR_BRA,
		INSTR_BRSL,
	};

	int i;

	for(i = 0; i < BRCNT; i++)
            if(static_cast<uint32_t>(inst->instr) == branch_instrs[i])
			return true;
	return false;
}
#undef BRCNT
