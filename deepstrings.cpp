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

struct str_chunk{
    void *lastupdateip;
    int len;
    int nulltermed;
};

// One str_chunk instance will be pointed to by both str_chunk_heads and str_chunk_tails
std::unordered_map<void *, struct str_chunk *> str_chunk_heads;
std::unordered_map<void *, struct str_chunk *> str_chunk_tails;
// Stores write operation details temporally
std::unordered_map<void *, struct wop_detail *> wopmap;

static void debugprint(VOID *ip){
    VOID *dst = wopmap[ip]->dst;
    INT32 size = wopmap[ip]->size;
    int i;

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
        for(i = 0; i < size; i++){
            fprintf(debugout, "%02x", ((unsigned char *)dst)[i]);
        }
    }
    fprintf(debugout, "\n");
}

static void printone(void *head){
    struct str_chunk *sc;
    if(str_chunk_heads.find(head) != str_chunk_heads.end()){
        sc = str_chunk_heads[head];
        if(sc->len >= 3 && sc->nulltermed){ // TODO: should be configurable
            fprintf(out, "%p %p %d: %s\n", sc->lastupdateip, head, sc->len, (char *)head);
        }
    }
}

//static void clear(void){
    // TODO: clear 3 maps and call in fini
//}

static int isasciicommon(unsigned char ch){
    return (0x20 <= ch && ch <= 0x7e) || ch == 0x09 || ch == 0x0a || ch == 0x0d;
}

static void _find_and_coalesce(void *ip, void *_dst, int size){
    int i, head;
    char *dst = (char *)_dst;
    struct str_chunk *sc, *sc2;
    int nulltermed;

    head = 0;
    // TODO: size + 1 or size should be configurable since it may cause sigsegv or sigbus.
    // TODO: signal handlers for sigbus and sigsegv
    for(i = 0; i < size; i++){
        // TODO: isascii* functions should be configurable
        if(!isasciicommon((unsigned char)dst[i])){
            nulltermed = dst[i] == '\0';
            if(head == 0){
                auto prevchunk = str_chunk_tails.find(dst - 1);
                if(prevchunk != str_chunk_tails.end()){
                    sc = str_chunk_tails[dst - 1];
                    if(i > 0){
                        sc->lastupdateip = ip;
                        sc->len += i;
                        sc->nulltermed = nulltermed;
                        str_chunk_tails.erase(dst - 1);
                        str_chunk_tails[dst + i - 1] = sc;
                        printone(dst + i - sc->len);
                    }else{
                        if(sc->nulltermed != nulltermed){
                            sc->lastupdateip = ip;
                            sc->nulltermed = nulltermed;
                            printone(dst - sc->len);
                        }
                    }
                }else{
                    if(i > 0){
                        sc = (struct str_chunk *)malloc(sizeof(struct str_chunk));
                        sc->lastupdateip = ip;
                        sc->len = i;
                        sc->nulltermed = nulltermed;
                        str_chunk_heads[dst] = sc;
                        str_chunk_tails[dst + i - 1] = sc;
                        printone(dst);
                    }
                }
            }else{
                if(i > head){
                    sc = (struct str_chunk *)malloc(sizeof(struct str_chunk));
                    sc->lastupdateip = ip;
                    sc->len = i - head;
                    sc->nulltermed = nulltermed;
                    str_chunk_heads[dst + head] = sc;
                    str_chunk_tails[dst + i - 1] = sc;
                    printone(dst + head);
                }
            }
            head = i + 1;
        }
    }

    if(head < size){
        auto nextchunk = str_chunk_heads.find(dst + size);
        if(nextchunk != str_chunk_heads.end()){
            if(head == 0 && str_chunk_tails.find(dst - 1) != str_chunk_tails.end()){
                // a case where coalescing with both prev and next chunks
                sc = str_chunk_tails[dst - 1];
                sc2 = str_chunk_heads[dst + size];
                sc->lastupdateip = ip;
                sc->len += size + sc2->len;
                sc->nulltermed = sc2->nulltermed;
                str_chunk_tails.erase(dst - 1);
                str_chunk_heads.erase(dst + size);
                str_chunk_tails.erase(dst + size + sc2->len - 1);
                str_chunk_tails[dst + size + sc2->len - 1] = sc;
                free(sc2);
                printone(dst + size + sc2->len - sc->len);
            }else{
                sc = str_chunk_heads[dst + size];
                sc->lastupdateip = ip;
                sc->len += size - head;
                str_chunk_heads.erase(dst + size);
                str_chunk_heads[dst + head] = sc;
                printone(dst + head);
            }
        }else{ // do not coalesce with the next if no next chunk exists even if the trailing char is ASCII
            if(head == 0 && str_chunk_tails.find(dst - 1) != str_chunk_tails.end()){
                sc = str_chunk_tails[dst - 1];
                sc->lastupdateip = ip;
                sc->len += size;
                sc->nulltermed = 0;
                str_chunk_tails.erase(dst - 1);
                str_chunk_tails[dst + size - 1] = sc;
                printone(dst + size - sc->len);
            }else{
                sc = (struct str_chunk *)malloc(sizeof(struct str_chunk));
                sc->lastupdateip = ip;
                sc->len = size - head;
                sc->nulltermed = 0;
                str_chunk_heads[dst + head] = sc;
                str_chunk_tails[dst + size - 1] = sc;
                printone(dst + head);
            }
        }
    }
}

static void find_and_coalesce(VOID *ip){
    _find_and_coalesce(ip, wopmap[ip]->dst, wopmap[ip]->size);
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
    find_and_coalesce(ip);
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
