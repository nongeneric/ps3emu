/****************************************************************************/
//
// Copyright SN Systems Ltd 2011
//
// Description: BasicExamples(a).c is a set of simple examples designed to 
// illustrate some of the PS3Debugger scripting functionality. The script  
// registers menu items in the Source and Memory Panes
// making frequent use of GetObjectData to determine processID, threadID or address
// parameters.
//
//****************************************************************************/

char tolower(char ch)
{
	if ( 0x41 <= (sn_uint32)ch && (sn_uint32)ch <=0x60)
		return (char)((sn_uint32)ch + 32);
	return ch;
}
////////////////////////////////////////////////////
void print_expression(sn_val Expr)
{
	switch(Expr.type)
	{
	case snval_int8:
		printf("\n8 bit 0x%x", (sn_uint32) Expr.val.i8);
		break;
	case snval_uint8:
		printf("\n8 bit 0x%x", (sn_uint32) Expr.val.u8);
		break;
	case snval_int16:
		printf("\n16 bit 0x%x", (sn_uint32) Expr.val.i16);
		break;
	case snval_uint16:
		printf("\n16 bit 0x%x", (sn_uint32) Expr.val.u16);
		break;
	case snval_int32:
		printf("\n32 bit 0x%x", Expr.val.i32);
		break;
	case snval_uint32:
		printf("\n32 bit 0x%x", Expr.val.u32);
		break;
	case snval_int64:
		printf("\n64 bit 0x%08x%08x",  Expr.val.i64.word[1], Expr.val.i64.word[0]);
		break;
	case snval_uint64:
		printf("\n64 bit 0x%08x%08x",  Expr.val.u64.word[1], Expr.val.u64.word[1]);
		break;
	case snval_int128:
		printf("\n128 bit 0x%08x%08x%08x%08x", Expr.val.i128.word[3], Expr.val.i128.word[2], Expr.val.i128.word[1], Expr.val.i128.word[0]);
		break;
	case snval_uint128:
		printf("\n128 bit 0x%08x%08x%08x%08x", Expr.val.u128.word[3], Expr.val.u128.word[2], Expr.val.u128.word[1], Expr.val.u128.word[0]);
		break;
	case snval_f32:
		printf("\nFloat %f",Expr.val.f32);
		break;
	case snval_f64:
		printf("\nFloat %f", Expr.val.f64);
		break;
	case snval_address:
		printf("\nAddress is %04x%04x", Expr.val.Address.word[1], Expr.val.Address.word[0]);
		break;
	default:
		printf("\nUnrecognised type\n");
	}
	return;
}
////////////////////////////////////////////////////
sn_int32 GetMemory(sn_uint32 ProcessID, sn_uint64 ThreadID, unsigned char *pBuffer, sn_address address, sn_uint32 uCount)
{
	int i;

	if (SN_FAILED(PS3GetMemory(ProcessID, ThreadID, pBuffer, address, uCount)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

  printf("\n0x");
	for(i=0;i<4;i++) printf("%02X", pBuffer[i]);

	return 1;
}
////////////////////////////////////////////////////
sn_int32 SetMemory(sn_uint32 ProcessID, sn_uint64 ThreadID, sn_uint8 *pBuffer, sn_address address, sn_uint32 uCount)
{
	if (SN_FAILED(PS3SetMemory(ProcessID,ThreadID,pBuffer,address,uCount)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}
	return 1;
}
////////////////////////////////////////////////////
sn_int32 GetBreakPoints(sn_uint32 ProcessID, sn_uint64 ThreadID)
{
	int i;
	sn_address address[20];
	sn_uint32 NumBP;

	if (SN_FAILED(PS3GetBreakPoints(ProcessID, ThreadID, address, &NumBP)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}
	for(i=0;i<NumBP;i++) printf("\nBreakPoint at 0x%08X", address[i].word[0]);

	return 1;
}
////////////////////////////////////////////////////
sn_uint32 GetRawSPUInfo(sn_uint32 uProcessID, sn_uint64 pThreadID)
{
	SNPS3_RAW_SPU_INFO* pRawInfo = NULL;
	sn_uint32 uBuffSize;

	if (SN_FAILED(PS3GetRawSPUInfo(uProcessID,pThreadID, pRawInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

	pRawInfo = (SNPS3_RAW_SPU_INFO*)malloc(uBuffSize);

	if (SN_FAILED(PS3GetRawSPUInfo(uProcessID,pThreadID, pRawInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pRawInfo);
		return 0;
	}
	printf("\nRaw SPU Info received");
	printf("\n\tID: 0x%x", pRawInfo->uID);
	printf("\n\tParent ID: 0x%x", pRawInfo->uParentID);
	printf("\n\tLast PC Address: 0x%08x%08x", pRawInfo->aLastPC.word[1], pRawInfo->aLastPC.word[0]);

	if (pRawInfo->uNameLen != 0)
		printf("\n\tThread Name: %s", ((char*)pRawInfo + sizeof(SNPS3_RAW_SPU_INFO)));
	if (pRawInfo->uFileNameLen != 0)
		printf("\n\tFile Name: %s", ((char*)pRawInfo + sizeof(SNPS3_RAW_SPU_INFO) + pRawInfo->uNameLen));

	return 1;
}
////////////////////////////////////////////////////
sn_int32 GetPPUThreadInfo(sn_uint32 uProcessID, sn_uint64 pThreadID)
{
	SNPS3_PPU_THREAD_INFO* pThreadInfo = NULL;
	sn_uint32 uBuffSize;

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

	pThreadInfo = (SNPS3_PPU_THREAD_INFO*)malloc(uBuffSize);

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pThreadInfo);
		return 0;
	}
	printf("\nPPU Thread Info received");
	printf("\n\tThread ID: 0x%08x", pThreadInfo->uThreadID.word[0]);
	printf("\n\tPriority 0x%x", pThreadInfo->uPriority);
	printf("\n\tStack Address: 0x%08x", pThreadInfo->uStackAddress.word[0]);
	printf("\n\tStack Size: 0x%08x", pThreadInfo->uStackSize.word[0]);
	printf("\n\tThread Name: %s", ((char*)pThreadInfo + sizeof(SNPS3_PPU_THREAD_INFO)));
	switch(pThreadInfo->uState){
		case(PPU_STATE_IDLE):
			printf("\n\tIdle");
			break;
		case(PPU_STATE_EXECUTABLE):
			printf("\n\tExecutable");
			break;
		case(PPU_STATE_EXECUTING):
			printf("\n\tExecuting");
			break;
		case(PPU_STATE_SLEEPING):
			printf("\n\tSleeping");
			break;
		case(PPU_STATE_SUSPENDED):
			printf("\n\tSuspended");
			break;
		case(PPU_STATE_STOPPED_BUSY):
			printf("\n\tStopped - BUSY");
			break;
		case(PPU_STATE_STOPPED):
			printf("\n\tStopped");
			break;
		default:
			printf("\n\tUnknown");
	}

	free(pThreadInfo);
	return 1;
}
////////////////////////////////////////////////////
sn_int32 GetSPUThreadInfo(sn_uint32 uProcessID, sn_uint64 pThreadID)
{
	SNPS3_SPU_THREAD_INFO* pThreadInfo = NULL;
	sn_uint32 uBuffSize;

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

	pThreadInfo = (SNPS3_SPU_THREAD_INFO*)malloc(uBuffSize);

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pThreadInfo);
		return 0;
	}
	printf("\nSPU Thread Info received");
	printf("\n\tThread ID: 0x%x%08x", pThreadInfo->uThreadID.word[1], pThreadInfo->uThreadID.word[0]);
	printf("\n\tThread Group ID: 0x%x%08x", pThreadInfo->uThreadGroupID.word[1], pThreadInfo->uThreadGroupID.word[0]);
	if (pThreadInfo->uThreadNameLen != 0)
		printf("\n\tThread Name: %s", ((char*)pThreadInfo + sizeof(SNPS3_SPU_THREAD_INFO)));
	if (pThreadInfo->uFileNameLen != 0)
		printf("\n\tFile Name: %s", ((char*)pThreadInfo + sizeof(SNPS3_SPU_THREAD_INFO) + pThreadInfo->uThreadNameLen));

	free(pThreadInfo);
	return 1;
}
////////////////////////////////////////////////////
sn_int32 GetSPUThreadGroupInfo(sn_uint32 uProcessID, sn_uint64 pThreadID)
{
	SNPS3_SPU_THREAD_INFO* pThreadInfo = NULL;
	sn_uint32 uBuffSize;

	SNPS3_SPU_THREADGROUP_INFO* pThreadGroupInfo = NULL;
	sn_uint32 uBuffSizeG;

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

	pThreadInfo = (SNPS3_SPU_THREAD_INFO*)malloc(uBuffSize);

	if (SN_FAILED(PS3GetThreadInfo(uProcessID,pThreadID, pThreadInfo, &uBuffSize)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pThreadInfo);
		return 0;
	}

	printf("\n\tThread Group ID: 0x%x%08x", pThreadInfo->uThreadGroupID.word[1], pThreadInfo->uThreadGroupID.word[0]);

	if (SN_FAILED(PS3GetSPUThreadGroupInfo(uProcessID, pThreadInfo->uThreadGroupID, pThreadGroupInfo, &uBuffSizeG)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pThreadInfo);
		return 0;
	}
	pThreadGroupInfo = (SNPS3_SPU_THREADGROUP_INFO*)malloc(uBuffSizeG);

	if (SN_FAILED(PS3GetSPUThreadGroupInfo(uProcessID, pThreadInfo->uThreadGroupID, pThreadGroupInfo, &uBuffSizeG)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pThreadGroupInfo);
		free(pThreadInfo);
		return 0;
	}
	printf("\n Thread Group 0x%x%08x Info:", pThreadGroupInfo->uThreadGroupID.word[1], pThreadGroupInfo->uThreadGroupID.word[0]);
	printf("\n\tPriority:%x ", pThreadGroupInfo->uPriority);
	printf("\n\tNum Threads:%x ", pThreadGroupInfo->uNumThreads);
	printf("\n\tName:%s", ((char*)pThreadGroupInfo + sizeof(SNPS3_SPU_THREADGROUP_INFO)));
	switch(pThreadGroupInfo->uState){
		case(SPU_STATE_NOT_CONFIGURED):
			printf("\n\tNot Configured");
			break;
		case(SPU_STATE_CONFIGURED):
			printf("\n\tConfigured");
			break;
		case(SPU_STATE_READY):
			printf("\n\tReady");
			break;
		case(SPU_STATE_WAITING):
			printf("\n\tWaiting");
			break;
		case(SPU_STATE_SUSPENDED):
			printf("\n\tSuspended");
			break;
		case(SPU_STATE_WAITING_SUSPENDED):
			printf("\n\tWaiting & Suspended");
			break;
		case(SPU_STATE_RUNNING):
			printf("\n\tRunning");
			break;
		case(SPU_STATE_STOPPED):
			printf("\n\tStopped");
			break;
		default:
			printf("\n\tUnknown");
			break;
	}

	free(pThreadInfo);
	free(pThreadGroupInfo);
	return 1;
}
////////////////////////////////////////////////////
sn_int32 GetProcessInfo(sn_uint32 uProcessID)
{
	SNPS3_PROCESS_INFO* pProcessInfo = NULL;
	sn_uint32 uBufferSize;
	int i;

	if (SN_FAILED(PS3GetProcessInfo(uProcessID, pProcessInfo, &uBufferSize, NULL)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}

	pProcessInfo = (SNPS3_PROCESS_INFO*)malloc(sizeof(SNPS3_PROCESS_INFO));
	pProcessInfo->ThreadIDs = (sn_uint64*)malloc(uBufferSize);

	if (SN_FAILED(PS3GetProcessInfo(uProcessID, pProcessInfo, &uBufferSize, pProcessInfo->ThreadIDs)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pProcessInfo->ThreadIDs);
		free(pProcessInfo);
		return 0;
	}

	printf("\nProcess Info:");
	printf("\n\t Num PPU Threads: %x", pProcessInfo->Hdr.uNumPPUThreads);
	printf("\n\t Num SPU Threads: %x", pProcessInfo->Hdr.uNumSPUThreads);
	printf("\n\t Parent Process ID: %x", pProcessInfo->Hdr.uParentProcessID);
	printf("\n\t Max Mem Size: 0x%x%08x", pProcessInfo->Hdr.uMaxMemorySize.word[1], pProcessInfo->Hdr.uMaxMemorySize.word[0]);
	printf("\nPath: %s", pProcessInfo->Hdr.szPath);

	for(i=0;i<(pProcessInfo->Hdr.uNumPPUThreads+pProcessInfo->Hdr.uNumSPUThreads);i++)
	{
		printf("\n\tThreadID: 0x%x%08x", pProcessInfo->ThreadIDs[i].word[1], pProcessInfo->ThreadIDs[i].word[0]);
	}

	switch(pProcessInfo->Hdr.uStatus){
		case(PS_STATE_CREATING):
			printf("\n\tState Creating");
			break;
		case(PS_STATE_READY):
			printf("\n\tState Ready");
			break;
		case(PS_STATE_EXITED):
			printf("\n\tState Exited");
			break;
		case(PS_STATE_UNKNOWN):
			printf("\n\tState Unknown");
			break;
		default:
			printf("\n\tInvalid State");
			break;
	}


	free(pProcessInfo->ThreadIDs);
	free(pProcessInfo);
	return 1;
}
////////////////////////////////////////////////////
sn_uint32 AddressToLine(sn_uint32 uProcessID, sn_uint64 uThreadID, sn_address address)
{
	char *pName = NULL;
	sn_uint32 uLen;
	sn_uint32 uLine;

	if (SN_FAILED(PS3AddressToLine(uProcessID, uThreadID, address, pName, &uLen, &uLine)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}
	pName = (char*)malloc(uLen * sizeof(char));

	if (SN_FAILED(PS3AddressToLine(uProcessID, uThreadID, address, pName, &uLen, &uLine)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pName);
		return 0;
	}

	printf("\nInfo from Address 0x%X", address.word[0]);
	printf("\n\tLine Number %d", uLine);
	printf("\n\tName %s", pName);

	free(pName);
	return 0;
}
////////////////////////////////////////////////////
sn_uint32 AddressToLabel(sn_uint32 uProcessID, sn_uint64 uThreadID, sn_address address)
{

	char *pName = NULL;
	sn_uint32 uLen;

	if (SN_FAILED(PS3AddressToLabel(uProcessID, uThreadID, address, pName, &uLen)))
	{
		printf("Error: %s", GetLastErrorString());
		return 0;
	}
	pName = (char*)malloc(uLen * sizeof(char));

	if (SN_FAILED(PS3AddressToLabel(uProcessID, uThreadID, address, pName, &uLen)))
	{
		printf("Error: %s", GetLastErrorString());
		free(pName);
		return 0;
	}

	printf("\nInfo from Address 0x%x%08x", address.word[0], address.word[1]);
	printf("\n\tName %s", pName);
	free(pName);
	return 0;
}

static char filePath[1024];
static char fileName[1024];

const char* regstrs[32] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", 
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", 
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", 
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
};
const char* vmregstrs[32] = {
	"v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", 
	"v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
	"v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", 
	"v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
};
const char* spuregstrs[128] = {
	"r000", "r001", "r002", "r003", "r004", "r005", "r006", "r007", "r008", "r009", "r010", 
	"r011", "r012", "r013", "r014", "r015", "r016", "r017", "r018", "r019", "r020", "r021",
	"r022", "r023", "r024", "r025", "r026", "r027", "r028", "r029", "r030", "r031", "r032", 
	"r033", "r034", "r035", "r036", "r037", "r038", "r039", "r040", "r041", "r042", "r043", 
	"r044", "r045", "r046", "r047", "r048", "r049", "r050", "r051", "r052", "r053", "r054", 
	"r055", "r056", "r057", "r058", "r059", "r060", "r061", "r062", "r063", "r064", "r065", 
	"r066", "r067", "r068", "r069", "r070", "r071", "r072", "r073", "r074", "r075", "r076", 
	"r077", "r078", "r079", "r080", "r081", "r082", "r083", "r084", "r085", "r086", "r087", 
	"r088", "r089", "r090", "r091", "r092", "r093", "r094", "r095", "r096", "r097", "r098", 
	"r099", "r100", "r101", "r102", "r103", "r104", "r105", "r106", "r107", "r108", "r109", 
	"r110", "r111", "r112", "r113", "r114", "r115", "r116", "r117", "r118", "r119", "r120",
	"r121", "r122", "r123", "r124", "r125", "r126", "r127"
};

sn_uint32 get_pc(sn_uint32 process, sn_uint64 thread) {
	sn_val result;
	if (SN_FAILED(PS3EvaluateExpression(process, thread, &result, "pc"))) {
		printf("\nError: %s", GetLastErrorString());
		return -1;
	}
	return result.val.i64.word[0];
}

sn_address get_pc_addr(sn_uint32 process, sn_uint64 thread) {
	sn_address addr;
	addr.word[0] = get_pc(process, thread);
	addr.word[1] = 0;
	return addr;
}

void runto(sn_uint32 process, sn_uint64 thread, sn_uint32 to) {
	sn_address to_address;
	to_address.word[0] = to;
	
	PS3AddBreakPoint(process, thread, to_address, 0);
	PS3ThreadStart(process, thread);
	while (to_address.word[0] != get_pc(process, thread)) {
	}
	PS3RemoveAllBreakPoints(process, thread);
}

// PS3ThreadStep doesn't work for SPU threads
// this function emulates single stepping by interpreting
// branch instructions and using breakpoints to stop at 
// a predicted next instruction
void step_in(sn_uint32 process, sn_uint64 thread) {
	sn_val result;
	sn_address pc;
	sn_uint32 instr;
	sn_uint32 opcode9;
	sn_uint32 opcode11;
	sn_uint32 rt;
	sn_uint32 ra;
	sn_int16 i16_shifted2;
	
	pc = get_pc_addr(process, thread);
	
	PS3GetMemory(process, thread, &instr, pc, 4);
	instr = EndianSwap_4(instr);
	opcode9 = instr >> (32 - 9) & 0x1ff;
	opcode11 = instr >> (32 - 11) & 0x7ff;
	i16_shifted2 = ((instr >> 7) << 2) & 0xffff;
	rt = instr & 0x7f;
	ra = (instr >> 7) & 0x7f;
	
	if (opcode9 == 0x64 || opcode9 == 0x60 || opcode9 == 0x66 ||
	    opcode9 == 0x62 || opcode9 == 0x42 || opcode9 == 0x40 ||
		opcode9 == 0x46 || opcode9 == 0x44 || opcode11 == 0x1a9 ||
		opcode11 == 0x128 || opcode11 == 0x129 || opcode11 == 0x12a ||
		opcode11 == 0x12b || opcode11 == 0x1a8 || opcode11 == 0x1aa) {
		
		switch (opcode11) {
			case 0x1a9:
			case 0x1a8: {
				PS3EvaluateExpression(process, thread, &result, spuregstrs[ra]);
				pc.word[0] = result.val.i128.word[3];
				printf("bi to %08x\n", pc.word[0]);
				break;
			}
			case 0x129:
			case 0x128: {
				PS3EvaluateExpression(process, thread, &result, spuregstrs[rt]);
				if ((result.val.i128.word[3] == 0 && opcode11 == 0x128) ||
				    (result.val.i128.word[3] != 0 && opcode11 == 0x129)) {
					PS3EvaluateExpression(process, thread, &result, spuregstrs[ra]);
					pc.word[0] = (result.val.i128.word[3] & 0xfffffffc);
					printf("biz/binz to %08x\n", pc.word[0]);
				}
				break;
			}
			default: {
				switch (opcode9) {
					case 0x46:
					case 0x40:
					case 0x44:
					case 0x42: {
						PS3EvaluateExpression(process, thread, &result, spuregstrs[rt]);
						if ((result.val.i128.word[3] != 0 && opcode9 == 0x42) ||
							(result.val.i128.word[3] == 0 && opcode9 == 0x40) ||
							((result.val.i128.word[3] & 0xffff) != 0 && opcode9 == 0x46) ||
							((result.val.i128.word[3] & 0xffff) == 0 && opcode9 == 0x44)) {
							pc.word[0] += i16_shifted2;
							printf("brnz/brz to %08x\n", pc.word[0]);
						} else {
							pc.word[0] += 4;
						}
						break;
					}
					case 0x66:
					case 0x64: {
						pc.word[0] += i16_shifted2;
						printf("br/brsl to %08x\n", pc.word[0]);
						break;
					}
					default: 
						printf("unsupported branch!");
						return;
				}
			}
		}
	} else {
		pc.word[0] += 4;
	}
	
	PS3AddBreakPoint(process, thread, pc, 0);
	PS3ThreadStart(process, thread);
	
	while (pc.word[0] != get_pc(process, thread)) {
	}
	
	PS3RemoveAllBreakPoints(process, thread);
}

void step_over(sn_uint32 process, sn_uint64 thread) {
	sn_val result;
	sn_address * pAddress;
	sn_uint32 pc;
	
	pAddress = malloc(8);
	
	pc = get_pc(process, thread) + 4;
	
	pAddress->word[0] = pc;
	pAddress->word[1] = 0;
	
	PS3ThreadRunToAddress(process, thread, *pAddress);
	
	while (pc != get_pc(process, thread)) {
	}
}

void spu_traceto(unsigned long to, unsigned long* steps, sn_uint32 uProcessID, sn_uint64 uThreadID, FILE* f) {
	unsigned long pc;
	sn_val result;
	int i;
	
	while (pc != to) {
		pc = get_pc(uProcessID, uThreadID);
		
		fprintf(f, "pc:%08x;", pc);
		fflush(f);
		
		for (i = 0; i < 128; i++) {
			if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, spuregstrs[i]))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			fprintf(f, "r%03d:%08x%08x%08x%08x;", i, 
				result.val.i128.word[3],
				result.val.i128.word[2],
				result.val.i128.word[1],
				result.val.i128.word[0]);
			fflush(f);
		}
		
		fprintf(f, "\n");
		
		step_in(uProcessID, uThreadID);
		
		if (*steps % 100 == 0) {
			printf("steps = %d\n", *steps);
		}
		
		(*steps)++;
		
		pc = get_pc(uProcessID, uThreadID);
	}
}

void ppu_traceto(unsigned long to, unsigned long* steps, sn_uint32 uProcessID, sn_uint64 uThreadID, FILE* f) {
	unsigned long pc, oldpc;
	sn_val result;
	int i;
	
	while (pc != to) {
		PS3UpdateTargetInfo();
		
		if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, "pc"))) {
			printf("\nError: %s", GetLastErrorString());
			break;
		}
		
		pc = result.val.i64.word[0];
		oldpc = pc;
		
		fprintf(f, "pc:%08x;", pc);
		fflush(f);
		
		for (i = 0; i < 32; i++) {
			if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, regstrs[i]))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			fprintf(f, "r%d:%08x%08x;", i, result.val.i64.word[1], result.val.i64.word[0]);
			fflush(f);
		}
		
		for (i = 0; i < 32; i++) {
			if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, vmregstrs[i]))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			fprintf(f, "v%d:%08x%08x%08x%08x;", i, 
				result.val.i128.word[3],
				result.val.i128.word[2],
				result.val.i128.word[1],
				result.val.i128.word[0]);
			fflush(f);
		}
		
		fprintf(f, "\n");
		
		PS3UpdateTargetInfo();
		
		if (SN_FAILED(PS3ThreadStep(uProcessID, uThreadID, SM_DISASSEMBLY))) {
			printf("\nError: %s", GetLastErrorString());
			break;
		}
		
		while (pc == oldpc) {
			if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, "pc"))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			pc = result.val.i64.word[0];
			PS3UpdateTargetInfo();
		}
		
		if (*steps % 100 == 0) {
			printf("steps = %d\n", *steps);
		}
		
		(*steps)++;
	}
}

int read_script_line(FILE* f, char* command, sn_uint32* address) {
	char buf[200];
	int i;
	
	fgets(buf, 200, f);
	i = 0;
	*address = 0;
	
	while (buf[i] == '\n' || buf[i] == ' ')
		i++;
	
	if (buf[i] != 't' && buf[i] != 's')
		return 1;
	*command = buf[i++];
	
	while (buf[i] == '\n' || buf[i] == ' ')
		i++;
	for (;;) {
		if (buf[i] < '0' || ('9' < buf[i] && buf[i] < 'A') || buf[i] > 'F')
			return 0;
		
		*address <<= 4;
		if ('0' <= buf[i] && buf[i] <= '9') {
			*address |= buf[i] - '0';
		} else {
			*address |= buf[i] - 'A' + 0xa;
		}
		i++;
	}
}

////////////////////////////////////////////////////
int main(int argc, char ** argv)
{
	sn_uint32 uProcessID;
	sn_uint64 uThreadID;
	unsigned char pBuffOut[4];

	sn_uint32 uSrcItemId[20];
	sn_uint32 uMemItemId[10];
	sn_uint32 uWatchItemId[10];
	sn_uint32 uLocalsItemId[10];
	sn_uint32 uAutosItemId[10];
	sn_uint32 uCallStackItemId[10];

	sn_notify Notify;
	sn_address * pAddress;
	BOOL bIsRunning;
	sn_uint32 uSize32 = 4;
	sn_uint32 uSize64 = 8;
	sn_uint32 i;
	sn_uint32* pBad;
	sn_uint32 ret_val;
	sn_val result;
	unsigned char pBuffIn[4];

	unsigned char pFileName[70];
	unsigned char pLabelName[128];
	sn_uint32 Length;
	sn_uint32 LineNumber;
	sn_uint32 NumUnits;

	char pInputBuffer[128];
	char *pExpressionInput;
	char *pNameInput;
	sn_uint32 uInputLength;

	BOOL bContinueEval;
	char *pName = NULL;
	sn_uint32 uLen;
	sn_uint32 uLine;
	
	unsigned long from;
	unsigned long to;
	unsigned long pc;
	unsigned long oldpc;
	sn_uint32 regs[64];
	sn_uint32 steps;
	sn_address sn_pc;
	sn_address addr;
	FILE* f;
	FILE* scriptf;
	const char* fpath = "/c/file.txt";
	char chr;
	unsigned long nextpc;

	pAddress = malloc(uSize64);
	pBuffIn[0] = 0xDE;
	pBuffIn[1] = 0xAD;
	pBuffIn[2] = 0xBE;
	pBuffIn[3] = 0xEF;
	
	AddMenuItem("!---------- Trace PPU (script)", &uSrcItemId[0], "MemoryView");
	AddMenuItem("!---------- Trace SPU (script)", &uSrcItemId[1], "MemoryView");
	AddMenuItem("!---------- Single Step", &uSrcItemId[2], "DisasmView");
	AddMenuItem("!---------- Eval", &uSrcItemId[3], "DisasmView");

	RequestNotification(NT_CUSTMENU);
	RequestNotification(NT_KEYDOWN);        //  Allows exit from script
	TextClear();

	printf("tracer loaded\n");

	while (1)
	{

		GetNotification(NT_ANY, &Notify);
		PS3UpdateTargetInfo();
		if ((sn_uint32)Notify.pParam1 == uSrcItemId[3]) {
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			
			if (SN_FAILED(PS3EvaluateExpression(uProcessID, uThreadID, &result, "*$pc = 200"))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			printf("%08x\n", result.val.i64.word[0]);
		}		
		else if ((sn_uint32)Notify.pParam1 == uSrcItemId[2]) {
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			
			step_in(uProcessID, uThreadID);
		} 
		else if ((sn_uint32)Notify.pParam1 == uSrcItemId[1])
		{
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			printf("\nEvaluate in the Thread 0x%x and Process %x context\n", uThreadID.word[0], uProcessID);
			
			//printf(">>");
			//GetInputLine(pInputBuffer, sizeof(pInputBuffer)/sizeof(char), &uInputLength);
			//to = strtoul(pInputBuffer, NULL, 16);
			//puts("\n");
			
			f = fopen("C:\\Users\\tr\\Desktop\\ps3realtrace.txt", "w");
			if (!f) {
				printf("failed to open file %s\n", fpath);
				break;
			}
			
			pc = 0;
			steps = 1;
			
			scriptf = fopen("C:\\Users\\tr\\Desktop\\ps3_spu_trace_script.txt", "r");
			
			while (!read_script_line(scriptf, &chr, &to)) {
				if (chr == 's') { // skip
					printf("skipping to %x\n", to);
					runto(uProcessID, uThreadID, to);
				} else { // 't' trace
					printf("tracing to %x\n", to);
					spu_traceto(to, &steps, uProcessID, uThreadID, f);
				}
			}
			
			fclose(scriptf);
			fclose(f);
		}
		if ((sn_uint32)Notify.pParam1 == uSrcItemId[0])
		{
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			printf("\nEvaluate in the Thread 0x%x and Process %x context\n", uThreadID.word[0], uProcessID);
			
			f = fopen("C:\\Users\\tr\\Desktop\\ps3realtrace.txt", "w");
			if (!f) {
				printf("failed to open file %s\n", fpath);
				break;
			}
			
			pc = 0;
			steps = 1;
			
			scriptf = fopen("C:\\Users\\tr\\Desktop\\ps3_ppu_trace_script.txt", "r");
			
			while (!read_script_line(scriptf, &chr, &to)) {
				if (chr == 's') { // skip
					printf("skipping to %x\n", to);
					runto(uProcessID, uThreadID, to);
				} else { // 't' trace
					printf("tracing to %x\n", to);
					ppu_traceto(to, &steps, uProcessID, uThreadID, f);
				}
			}
			
			fclose(scriptf);
			fclose(f);
		}
	}
	free(pAddress);
	return 0;
}
////////////////////////////////////////////////////
