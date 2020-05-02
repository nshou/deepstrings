#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include "pin.H"

static FILE *out;
static unsigned long maxlen = 128;
static unsigned long minlen = 3; //TODO: should be configurable

static int iscommonascii(unsigned char ch){
    return (0x20 <= ch && ch <= 0x7e) || ch == 0x09 || ch == 0x0a || ch == 0x0d;
}

static VOID onread(VOID *ip, VOID *addr){
    unsigned long i;
    char *c = (char *)addr;

    for(i = 0; c[i] != 0; i++){ //TODO: must care about sigsegv and sigbus
        if(!iscommonascii((unsigned char)c[i]) || i > maxlen){
            return;
        }
    }

    if(i >= minlen){
        fprintf(out, "rdip:%p addr:%p len:%lu str:%s\n", ip, addr, i, c);
    }
}

VOID instruction(INS ins, VOID *v){
    if(INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins)){
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)onread,
            IARG_INST_PTR,
            IARG_MEMORYREAD_EA,
            IARG_END);
    }
    if(INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins)){
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)onread,
            IARG_INST_PTR,
            IARG_MEMORYREAD2_EA,
            IARG_END);
    }
}

VOID fini(INT32 code, VOID *v){
    fprintf(out, "#eof\n");
    fclose(out);
}

int main(int argc, char *argv[]){
    out = fopen("deepstrings.out", "w");
    PIN_Init(argc, argv);

    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);

    // Never returns
    PIN_StartProgram();
    return 0;
}
