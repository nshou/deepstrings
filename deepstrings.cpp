#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "pin.H"

static FILE *out;
static unsigned long maxlen = 128;
static unsigned long floatlen = 48;
static unsigned long minlen = 8; //TODO: these should be configurable
static std::unordered_map<void *, char *> rop_history;
static std::unordered_map<char *, char *> data_history;
static char *floatstr;

static unsigned long bisect_right(std::vector<unsigned long> v, unsigned long x){
    unsigned long lo = 0;
    unsigned long hi = v.size();
    unsigned long mid;

    while(lo < hi){
        mid = (lo + hi) / 2;
        if(x < v[mid]){
            hi = mid;
        }else{
            lo = mid + 1;
        }
    }

    return lo;
}

static unsigned long bisect_left(std::vector<unsigned long> v, unsigned long x){
    unsigned long lo = 0;
    unsigned long hi = v.size();
    unsigned long mid;

    while(lo < hi){
        mid = (lo + hi) / 2;
        if(v[mid] < x){
            lo = mid + 1;
        }else{
            hi = mid;
        }
    }

    return lo;
}

class DCDict{
private:
    std::vector<unsigned long> keyblks;
    std::vector<void *> values;

public:
    void *get(unsigned long keyblk_l, unsigned long keyblk_r, void *dfl){
        unsigned long lpos;
        unsigned long rpos;

        if(keyblk_l > keyblk_r){
            return dfl;
        }

        lpos = bisect_right(keyblks, keyblk_l);
        rpos = bisect_left(keyblks, keyblk_r);

        if(keyblk_l == keyblk_r && lpos != rpos){
            // len of key to search is 1 and the key is on the edge of a block.
            return values[std::min(lpos, rpos) / 2];
        }else if(lpos == rpos && lpos % 2 == 1){
            return values[(lpos - 1) / 2];
        }

        return dfl;
    }

    void del(unsigned long keyblk_l, unsigned long keyblk_r){
        unsigned long lpos;
        unsigned long rpos;
        unsigned long ldel;
        unsigned long rdel;

        if(keyblk_l > keyblk_r){
            return;
        }

        lpos = bisect_left(keyblks, keyblk_l);
        rpos = bisect_right(keyblks, keyblk_r);

        if(lpos != rpos || lpos % 2 !=0){
            ldel = 2 * (lpos / 2);
            rdel = 2 * ((rpos + 1) / 2);
            keyblks.erase(keyblks.begin() + ldel, keyblks.begin() + rdel);
            values.erase(values.begin() + ldel / 2, values.begin() + rdel / 2);
            //TODO: free val
        }
    }

    void set(unsigned long keyblk_l, unsigned long keyblk_r, void *value){
        unsigned long pos;

        if(keyblk_l > keyblk_r){
            return;
        }

        del(keyblk_l, keyblk_r);
        pos = bisect_right(keyblks, keyblk_l);
        keyblks.insert(keyblks.begin() + pos, keyblk_r);
        keyblks.insert(keyblks.begin() + pos, keyblk_l);
        values.insert(values.begin() + pos / 2, value);
    }

    ~DCDict(){
        //TODO: free vals
    }
};

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
