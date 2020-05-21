#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include "pin.H"

static FILE *out;
static unsigned long maxlen = 128;
static unsigned long floatlen = 48;
static unsigned long minlen = 8; //TODO: these should be configurable
static std::unordered_map<void *, char *> rop_history;
static std::unordered_map<char *, char *> data_history;
static char *floatstr;

static int iscommonascii(unsigned char ch){
    return (0x20 <= ch && ch <= 0x7e) || ch == 0x09 || ch == 0x0a || ch == 0x0d;
}

static int dataredundancy(char *c){
    char *_c;
    int ret = 0;

    if(data_history.find(c) != data_history.end()){
        _c = data_history[c];
        if(strcmp(_c, c) == 0){
            ret = 1;
        }else{
            free(_c);
            data_history[c] = strdup(c);
        }
    }else{
        data_history[c] = strdup(c);
    }

    return ret;
}

static int operationalredundancy(void *ip, char *c){
    char *sub, *_c;
    int ret = 0;

    if(rop_history.find(ip) != rop_history.end()){
        _c = rop_history[ip];
        sub = strstr(_c, c);
        if(sub != NULL && sub == c && strlen(c) + (int)(sub - _c) == strlen(_c)){
            ret = 1;
        }
    }

    rop_history[ip] = c;
    return ret;
}

static int isredundant(void *ip, char *c){
    int datar = dataredundancy(c);
    int oper = operationalredundancy(ip, c);
    return datar || oper;
}

static VOID onread(VOID *ip, VOID *addr){
    unsigned long i;
    char *c = (char *)addr;

    for(i = 0; c[i] != 0 && i < maxlen; i++){ //TODO: must care about sigsegv and sigbus
        if(!iscommonascii((unsigned char)c[i])){
            if(i > floatlen){
                memcpy(floatstr, c, i);
                floatstr[i] = '\0';
                c = floatstr;
                break;
            }else{
                return;
            }
        }
    }

    if(i < minlen || i >= maxlen){
        return;
    }

    if(isredundant(ip, c)){
        return;
    }

    fprintf(out, "rdip:%p addr:%p len:%lu str:%s\n", ip, addr, i, c);
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
    floatstr = (char *)malloc(sizeof(char) * (maxlen + 1));

    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);
    PIN_StartProgram(); // Never returns

    return 0;
}
