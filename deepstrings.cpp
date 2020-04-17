#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include "pin.H"

static FILE *out;
static FILE *debugout;

struct wop_detail{
    VOID *dst;
    INT32 size;
};

// Stores write operation details temporally
std::unordered_map<void *, struct wop_detail *> wopmap;

static void debugprint(VOID *ip){
    VOID *dst = wopmap[ip]->dst;
    INT32 size = wopmap[ip]->size;

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

static VOID savewop(VOID *ip, VOID *addr, INT32 sz){
    struct wop_detail *wop = (struct wop_detail *)malloc(sizeof(struct wop_detail));
    wop->dst = addr;
    wop->size = sz;
    wopmap[ip] = wop;
}

static void forgetwop(VOID *ip){
    free(wopmap[ip]);
    wopmap.erase(ip);
}

static VOID memwriteop(VOID *ip){
    debugprint(ip);
    forgetwop(ip);
}

VOID instruction(INS ins, VOID *v){
    if(INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins)){
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)savewop,
                                 IARG_INST_PTR,
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
