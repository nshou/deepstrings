#include <stdio.h>
#include "pin.H"

static FILE *out;
static FILE *debugout;

// Variables to store values temporally
static VOID *dst;
static INT32 size;

static void debugprint(VOID *ip){
    fprintf(debugout, "ip:%p dst:%p sz:%d val:0x", ip, dst, size);
    if(size == 1){
        fprintf(debugout, "%x", *((unsigned char *)dst));
    }else if(size == 2){
        fprintf(debugout, "%x", *((unsigned short *)dst));
    }else if(size == 4){
        fprintf(debugout, "%x", *((unsigned int *)dst));
    }else if(size == 8){
        fprintf(debugout, "%lx", *((unsigned long *)dst));
    }else{
        for(int i = 0; i < size; i++){
            fprintf(debugout, "%02x", ((unsigned char *)dst)[i]);
        }
    }
    fprintf(debugout, "\n");
}

static VOID memwriteop(VOID *ip){
    debugprint(ip);
}

static VOID cachedata(VOID *addr, INT32 sz){
    dst = addr;
    size = sz;
}

VOID instruction(INS ins, VOID *v){
    if(INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins)){
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)cachedata,
                                 IARG_MEMORYWRITE_EA,
                                 IARG_MEMORYWRITE_SIZE,
                                 IARG_END);

        if(INS_IsValidForIpointAfter(ins)){
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)memwriteop,
                           IARG_INST_PTR,
                           IARG_END);
        }

        if(INS_IsValidForIpointTakenBranch(ins)){
            INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)memwriteop,
                           IARG_INST_PTR,
                           IARG_END);
        }
    }
}

VOID fini(INT32 code, VOID *v){
    fprintf(out, "#eof\n");
    fprintf(debugout, "#eof\n");
    fclose(out);
    fclose(debugout);
}

int main(int argc, char *argv[]){
    out = fopen("deepstrings.out", "w");
    debugout = fopen("deepstrings.out.debug", "w");
    PIN_Init(argc, argv);

    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);

    // Never returns
    PIN_StartProgram();
    return 0;
}
