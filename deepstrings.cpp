#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "pin.H"

#define DS_TOOL_SUMMARY "Deepstrings detects every string embedded in binary files"
#define DS_DFL_OUTPUT_FILE "deepstrings.out"
#define DS_DFL_OUTPUT_FILE_DESC "Specify output file name"
#define DS_DFL_MAXLEN "256"
#define DS_DFL_MAXLEN_DESC "Max length to search"

static FILE *output;
static unsigned long maxlen;
static unsigned long minlen = 8; //TODO: these should be configurable
static std::unordered_map<void *, char *> rop_history;
static std::unordered_map<char *, char *> data_history;
static char *emitbuffer;

KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", DS_DFL_OUTPUT_FILE, DS_DFL_OUTPUT_FILE_DESC);
KNOB<unsigned long> KnobMaxLen(KNOB_MODE_WRITEONCE, "pintool", "m", DS_DFL_MAXLEN, DS_DFL_MAXLEN_DESC);

struct chunk{
    char *headaddr;
    char *data;
};

struct emitinfo{
    void *insptr;
    char *headptr;
    unsigned long size;
};

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

    //TODO: free func registration

    ~DCDict(){
        //TODO: free vals
    }
};

DCDict chunks;

static int iscommonascii(unsigned char ch){
    return (0x20 <= ch && ch <= 0x7e) || ch == 0x09 || ch == 0x0a || ch == 0x0d;
}

static int isredundant(char *c, unsigned long i){
    unsigned long l, r;
    struct chunk *ck, *newck;

    l = (unsigned long)c;
    r = l + i - 1;

    ck = (struct chunk *)chunks.get(l, r, NULL);
    if(ck != NULL){
        if(memcmp(&(ck->data[c - ck->headaddr]), c, i) == 0){
            return 1;
        }
    }

    // TODO: free!
    newck = (struct chunk *)malloc(sizeof(struct chunk));
    newck->data = (char *)malloc(sizeof(char) * i);
    memcpy(newck->data, c, i);
    newck->headaddr = c;
    chunks.set(l, r, newck);
    return 0;
}

static void emit(struct emitinfo *ei){
    if(ei->headptr[ei->size - 1] == 0){
        ei->size--;
    }

    if(ei->size < minlen || ei->size > maxlen){
        return;
    }

    memcpy(emitbuffer, ei->headptr, ei->size);
    emitbuffer[ei->size] = 0;

    fprintf(output, "insptr:%p hdptr:%p len:%lu str:%s\n", ei->insptr, ei->headptr, ei->size, emitbuffer);
}

static VOID onread(VOID *ip, VOID *addr){
    struct emitinfo ei;
    unsigned long i = 0;
    char *c = (char *)addr;

    while(iscommonascii((unsigned char)c[i])){ //TODO: must care about sigsegv and sigbus
        i++;
    }

    if(c[i] == 0){
        // '\0' counts as a valid character but must split here as a chunk.
        i++;
    }

    if(i == 0){
        // Grabbed one non-ascii. Skip.
        return;
    }

    if(isredundant(c, i)){
        return;
    }

    ei.insptr = ip;
    ei.headptr = c;
    ei.size = i;
    emit(&ei);
}

static void usage(){
    fprintf(stderr, "%s\n%s", DS_TOOL_SUMMARY, KNOB_BASE::StringKnobSummary().c_str());
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
    fprintf(output, "#eof\n");
    fclose(output);
}

int main(int argc, char *argv[]){
    if(PIN_Init(argc, argv)){
        usage();
        return -1;
    }

    output = fopen(KnobOutputFile.Value().c_str(), "w");
    maxlen = KnobMaxLen.Value();
    emitbuffer = (char *)malloc(sizeof(char) * (maxlen + 1));

    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);
    PIN_StartProgram(); // Never returns

    return 0;
}
