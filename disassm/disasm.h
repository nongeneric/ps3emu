// Copyright 2011 naehrwert
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _DISASM_H_
#define _DISASM_H_

#include <stdint.h>
#include <vector>
#include <string>

//Instruction types.
#define SPU_INSTR_RR 1
#define SPU_INSTR_RRR 2
#define SPU_INSTR_RI7 3
#define SPU_INSTR_RI10 4
#define SPU_INSTR_RI16 5
#define SPU_INSTR_RI18 6
#define SPU_INSTR_SPECIAL 7
#define SPU_INSTR_NONE 8

//Instructions.
#define INSTR_NONE 0
#define INSTR_CDD 1
#define INSTR_ROTQMBII 2
#define INSTR_CLGT 3
#define INSTR_NAND 4
#define INSTR_IOHL 5
#define INSTR_GB 6
#define INSTR_MPYHH 7
#define INSTR_CDX 8
#define INSTR_ANDBI 9
#define INSTR_ORBI 10
#define INSTR_CLZ 11
#define INSTR_ABSDB 12
#define INSTR_BRHZ 13
#define INSTR_CNTB 14
#define INSTR_STOP 15
#define INSTR_CEQI 16
#define INSTR_CEQH 17
#define INSTR_BIZ 18
#define INSTR_CEQ 19
#define INSTR_CEQB 20
#define INSTR_ROTQBYI 21
#define INSTR_NOP 22
#define INSTR_SUMB 23
#define INSTR_NOR 24
#define INSTR_MPY 25
#define INSTR_DSYNC 26
#define INSTR_MPYS 27
#define INSTR_GBB 28
#define INSTR_MPYU 29
#define INSTR_GBH 30
#define INSTR_ROTI 31
#define INSTR_MPYA 32
#define INSTR_RDCH 33
#define INSTR_ROTM 34
#define INSTR_XSBH 35
#define INSTR_ILHU 36
#define INSTR_CGTI 37
#define INSTR_MPYH 38
#define INSTR_MPYI 39
#define INSTR_SHL 40
#define INSTR_BRSL 41
#define INSTR_CLGTHI 42
#define INSTR_SYNC 43
#define INSTR_HEQI 44
#define INSTR_CWX 45
#define INSTR_XOR 46
#define INSTR_ROTQMBI 47
#define INSTR_BIHZ 48
#define INSTR_CEQHI 49
#define INSTR_MPYHHAU 50
#define INSTR_AVGB 51
#define INSTR_ADDX 52
#define INSTR_ROTQMBY 53
#define INSTR_MFSPR 54
#define INSTR_STOPD 55
#define INSTR_XORHI 56
#define INSTR_CWD 57
#define INSTR_BG 58
#define INSTR_ORX 59
#define INSTR_BI 60
#define INSTR_CGX 61
#define INSTR_SFHI 62
#define INSTR_BR 63
#define INSTR_ORI 64
#define INSTR_ANDI 65
#define INSTR_ORC 66
#define INSTR_ILA 67
#define INSTR_XSWD 68
#define INSTR_ILH 69
#define INSTR_BISL 70
#define INSTR_ROTQMBYI 71
#define INSTR_BGX 72
#define INSTR_OR 73
#define INSTR_HBR 74
#define INSTR_BRZ 75
#define INSTR_SELB 76
#define INSTR_BRHNZ 77
#define INSTR_AHI 78
#define INSTR_CG 79
#define INSTR_HBRR 80
#define INSTR_MPYUI 81
#define INSTR_XORI 82
#define INSTR_FSMBI 83
#define INSTR_SHUFB 84
#define INSTR_BIHNZ 85
#define INSTR_BRA 86
#define INSTR_CHD 87
#define INSTR_RCHCNT 88
#define INSTR_FSMH 89
#define INSTR_MPYHHU 90
#define INSTR_XORBI 91
#define INSTR_LNOP 92
#define INSTR_FSMB 93
#define INSTR_ANDC 94
#define INSTR_EQV 95
#define INSTR_MPYHHA 96
#define INSTR_ROTMA 97
#define INSTR_CHX 98
#define INSTR_ROTHMI 99
#define INSTR_ROTMI 100
#define INSTR_CLGTBI 101
#define INSTR_CLGTB 102
#define INSTR_CLGTI 103
#define INSTR_CLGTH 104
#define INSTR_SHLI 105
#define INSTR_SHLQBII 106
#define INSTR_CEQBI 107
#define INSTR_SHLQBY 108
#define INSTR_SHLQBYI 109
#define INSTR_SHLQBI 110
#define INSTR_AND 111
#define INSTR_STQD 112
#define INSTR_CBD 113
#define INSTR_STQA 114
#define INSTR_AI 115
#define INSTR_AH 116
#define INSTR_ROTQBY 117
#define INSTR_HBRA 118
#define INSTR_STQR 119
#define INSTR_IL 120
#define INSTR_CBX 121
#define INSTR_MTSPR 122
#define INSTR_STQX 123
#define INSTR_CGT 124
#define INSTR_LQX 125
#define INSTR_LQR 126
#define INSTR_WRCH 127
#define INSTR_LQD 128
#define INSTR_CGTHI 129
#define INSTR_ROTMAI 130
#define INSTR_LQA 131
#define INSTR_SFX 132
#define INSTR_BRNZ 133
#define INSTR_ANDHI 134
#define INSTR_ORHI 135
#define INSTR_SFH 136
#define INSTR_SFI 137
#define INSTR_XSHW 138
#define INSTR_A 139
#define INSTR_FSM 140
#define INSTR_BINZ 141
#define INSTR_SF 142
#define INSTR_ROTQBII 143

#define REG_LR 0
#define REG_SP 1

typedef uint32_t u32;
typedef int32_t s32;

typedef struct _instr_t
{
	//Instruction name.
	const char *name;
	//Instruction type (SPU_INSTR_...).
	int type;
	//Instruction (INSTR_...).
	int instr;
	//Opcode.
	u32 opcode;
	//Address.
	u32 address;
	//Index in owning executable region.
	u32 idx;
	//Parameters.
	union
	{
		struct
		{
			u32 rt;
			u32 ra;
			u32 rb;
		} rr;
		struct
		{
			u32 rt;
			u32 ra;
			u32 rb;
			u32 rc;
		} rrr;
		struct
		{
			u32 rt;
			u32 ra;
			u32 i7;
		} ri7;
		struct
		{
			u32 rt;
			u32 ra;
			u32 i10;
		} ri10;
		struct
		{
			u32 rt;
			u32 i16;
		} ri16;
		struct
		{
			u32 rt;
			u32 i18;
		} ri18;
		struct
		{
			u32 opcode;
		} special;
	};
} instr_t;

#define BRANCH_TARGET(inst) (inst->address + inst->ri16.i16)
#define IIDX2ADDR(er, idx) (er->start + idx * sizeof(u32))

#define IS_RETURN(inst) (inst->instr == INSTR_BI && inst->rr.rt == REG_LR)

instr_t disasm_disassemble_instr(u32 instr);
std::string disasm_print_instr(instr_t *inst, bool print_addr);
bool disasm_is_branch(instr_t *inst);
bool disasm_is_direct_branch(instr_t *inst);
bool disasm_is_direct_cond_branch(instr_t *inst);
bool disasm_is_direct_uncond_branch(instr_t *inst);

#endif
