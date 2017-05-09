#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ppu_asm_intrinsics.h>
#include <ppu_altivec_internals.h>
#include <ppu_intrinsics.h>
#include <altivec.h>

void print(int i, void* val) {
	uint32_t* u = (uint32_t*)val;
	printf("%d: %08x %08x %08x %08x\n", i, u[0], u[1], u[2], u[3]);
}

#define _vmhraddshs(a,b,c) __extension__   \
  ({ vec_short8 __macro_result;		     \
  __asm__ ("vmhraddshs %0,%1,%2,%3"	     \
	   : "=v" (__macro_result)	     \
	   : "v" ((vec_short8) (a)),  \
	     "v" ((vec_short8) (b)),  \
		 "v" ((vec_short8) (c))); \
  __macro_result; })

int main() {
	vec_short8 a = { 0, 10, 20, 500, -500, 5000, -100000 };
	vec_short8 b = { -100, 500, 3300, 111111, -22222, -223322, -100000 };
	vec_short8 c = { 20, 50, 3000, 5000, -3000, -5000, -100000 };
	vec_short8 d = _vmhraddshs(a, b, c);
	print(0, &d);

	vec_short8 a1 = { 1, 0, -1, 5, 0xffff, 0x1ffff, 500, 1000 };
	vec_short8 b1 = { 0, 1, -1, 10, 20, 30, 400, 8000 };
	vec_uchar16 d1 = vec_vpkshus(a1, b1);
	print(1, &d1);
	
	vec_short8 a2 = { 1, 2, 3, 4, 5, 6, 7, 8 };
	vec_short8 d2 = vec_vsplth(a2, 0);
	print(2, &d2);
	d2 = vec_vsplth(a2, 7);
	print(3, &d2);
	d2 = vec_vsplth(a2, 2);
	print(4, &d2);
	d2 = vec_vsplth(a2, 5);
	print(5, &d2);
	d2 = vec_vsplth(a2, 4);
	print(6, &d2);

	vec_short8 a3 = { 0, 1, -1, 0xfff, 0x350, 0x30, 0x50, -500 };
	vec_short8 b3 = { 0, 15, -1000, 0xfff, 0x350, 5000, 300, 200 };
	vec_uchar16 d3 = vec_vpkshus(a, b);
	print(7, &d3);

	vec_short8 d4 = vec_vadduhm(a3, b3);
	print(8, &d4);

	vec_ushort8 d5 = vec_vslh((vec_ushort8)(a3), (vec_ushort8)(a3));
	print(9, &d5);
	d5 = vec_vslh((vec_ushort8)(a3), (vec_ushort8)(b3));
	print(10, &d5);
	d5 = vec_vslh((vec_ushort8)(a), (vec_ushort8)(b));
	print(11, &d5);
	d5 = vec_vslh((vec_ushort8)(a3), (vec_ushort8)(a2));
	print(12, &d5);
	d5 = vec_vslh((vec_ushort8)(a2), (vec_ushort8)(a3));
	print(13, &d5);

	vec_char16 a6 = { 0, 1, 10, 20, -1, -2, -10, -20, 1, 1, 2, 2, 3, 3, 4, 5 };
	vec_short8 d6 = vec_vupkhsb(a6);
	print(14, &d6);

	d6 = vec_vupklsb(a6);
	print(15, &d6);

	d5 = vec_vsrah((vec_ushort8)(a3), (vec_ushort8)(a3));
	print(16, &d5);
	d5 = vec_vsrah((vec_ushort8)(a3), (vec_ushort8)(b3));
	print(17, &d5);
	d5 = vec_vsrah((vec_ushort8)(a), (vec_ushort8)(b));
	print(18, &d5);
	d5 = vec_vsrah((vec_ushort8)(a3), (vec_ushort8)(a2));
	print(19, &d5);
	d5 = vec_vsrah((vec_ushort8)(a2), (vec_ushort8)(a3));
	print(20, &d5);

	vec_uchar16 a7 = { 0x4b, 0x48, 0x48, 0x4a, 0x4a, 0x4a, 0x4a, 0x4d, 0x4a, 0x48, 0x48, 0x4a, 0x4a, 0x4a, 0x4a, 0xff };
	vec_uchar16 b7 = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	vec_uchar16 c7 = { 0 };
	vec_uchar16 d7 = vec_vperm(c7, a7, b7);
	print(21, &d7);

	vec_ushort8 a8 = { 0xffca, 0xffca, 0xffcb, 0xffcc, 0xffff, 0xfffd, 0xffff, 0xfffe };
	vec_ushort8 b8 = { 0xffca, 0xffcb, 0xffcc, 0xffcc, 0xfffe, 0xfffd, 0xffff, 0xfffd };
	vec_ushort8 d8 = vec_vmrglh(a8, b8);	
	print(22, &d8);

	char arr[100] = {0};

	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)arr);
	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)(arr + 8));
	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)(arr + 16));
	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)(arr + 32));
	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)(arr + 34));
	__builtin_altivec_stvx((__vector signed int)(a6), 0, (void*)(arr + 40));
	print(23, arr);
	print(24, arr + 16);
	print(25, arr + 32);
	print(26, arr + 48);
	print(27, arr + 64);

	memset(arr, 0, 100);
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)arr);
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)(arr + 8));
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)(arr + 16));
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)(arr + 32));
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)(arr + 34));
	__builtin_altivec_stvxl((__vector signed int)(a6), 0, (void*)(arr + 40));
	print(28, arr);
	print(29, arr + 16);
	print(30, arr + 32);
	print(31, arr + 48);
	print(32, arr + 64);
}