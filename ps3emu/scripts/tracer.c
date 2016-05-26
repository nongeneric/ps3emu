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
	sn_uint32 steps;
	sn_address addr;
	FILE* f;
	const char* fpath = "/c/file.txt";

	pAddress = malloc(uSize64);
	pBuffIn[0] = 0xDE;
	pBuffIn[1] = 0xAD;
	pBuffIn[2] = 0xBE;
	pBuffIn[3] = 0xEF;
	
	//AddMenuItem("!---------- Trace To", &uSrcItemId[0], "DBGSourceView");
	//AddMenuItem("!---------- Step step", &uSrcItemId[1], "DBGSourceView");
	AddMenuItem("!---------- Trace To", &uSrcItemId[0], "MemoryView");
	AddMenuItem("!---------- Step step", &uSrcItemId[1], "MemoryView");

	RequestNotification(NT_CUSTMENU);
	RequestNotification(NT_KEYDOWN);        //  Allows exit from script
	TextClear();

	printf("tracer loaded\n");

	while (1)
	{

		GetNotification(NT_ANY, &Notify);
		PS3UpdateTargetInfo();
		if ((sn_uint32)Notify.pParam1 == uSrcItemId[1])
		{
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			GetObjectData(LOWORD(Notify.pParam2), GVD_ADDR_AT_CURSOR, pAddress, uSize64);
			puts("step step");
			if (SN_FAILED(PS3ThreadStep(uProcessID, uThreadID, SM_DISASSEMBLY))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
			PS3UpdateTargetInfo();
			if (SN_FAILED(PS3ThreadStep(uProcessID, uThreadID, SM_DISASSEMBLY))) {
				printf("\nError: %s", GetLastErrorString());
				break;
			}
		}
		if ((sn_uint32)Notify.pParam1 == uSrcItemId[0])
		{
			GetObjectData(LOWORD(Notify.pParam2), GVD_THREAD, &uThreadID, uSize64);
			GetObjectData(LOWORD(Notify.pParam2), GVD_PROCESS, &uProcessID, uSize32);
			printf("\nEvaluate in the Thread 0x%x and Process %x context\n", uThreadID.word[0], uProcessID);
			bContinueEval = TRUE;
			while(bContinueEval)
			{
				printf(">>");
				GetInputLine(pInputBuffer, sizeof(pInputBuffer)/sizeof(char), &uInputLength);
				to = strtoul(pInputBuffer, NULL, 16);
				puts("\n");
				
				//FileSelectionDialog(filePath, fileName);
				
				//printf("%s\n", filePath);
				//printf("%s\n", fileName);
				
				f = fopen("C:\\Users\\tr\\Desktop\\ps3realtrace.txt", "w");
				if (!f) {
					printf("failed to open file %s\n", fpath);
					break;
				}
				printf("\ntracing to %x\n", to);
				pc = 0;
				steps = 1;
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
					
					if (steps % 100 == 0) {
						printf("steps = %d\n", steps);
					}
					
					steps++;
				}
				
				fclose(f);
			}
			printf("Finished evaluating\n\n");
		}
	}
	free(pAddress);
	return 0;
}
////////////////////////////////////////////////////
