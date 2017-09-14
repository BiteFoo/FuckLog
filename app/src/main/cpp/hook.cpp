//
// Created by John.Lu on 2017/9/14.
//

/*
Copyright (C) 2013 Andrey Petrov

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


/**

android 免root hook 系统调用
https://stackoverflow.com/questions/23443848/how-to-hook-system-calls-of-my-android-app-non-rooted-device/27099442#27099442
具体参考
https://shadowwhowalks.blogspot.jp/2013/01/android-hacking-hooking-system.html

 hook_call("libandroid_runtime.so", "connect", &my_connect);

*/

#define ANDROID_ARM_LINKER
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/atomics.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "linker.h"  // get it from bionic
static unsigned elfhash(const char *_name)
{
    const unsigned char *name = (const unsigned char *) _name;
    unsigned h = 0, g;
    while(*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        h ^= g;
        h ^= g >> 24;
    }
    return h;
}
static Elf32_Sym *soinfo_elf_lookup(soinfo *si, unsigned hash, const char *name)
{
    Elf32_Sym *s;
    Elf32_Sym *symtab = si->symtab;
    const char *strtab = si->strtab;
    unsigned n;
    n = hash % si->nbucket;
    for(n = si->bucket[hash % si->nbucket]; n != 0; n = si->chain[n]){
        s = symtab + n;
        if(strcmp(strtab + s->st_name, name)) continue;
        return s;
    }
    return NULL;
}
int hook_call(char *soname, char *symbol, unsigned newval) {
    soinfo *si = NULL;
    Elf32_Rel *rel = NULL;
    Elf32_Sym *s = NULL;
    unsigned int sym_offset = 0;
    if (!soname || !symbol || !newval)
        return 0;
    si = (soinfo*) dlopen(soname, 0);
    if (!si)
        return 0;
    s = soinfo_elf_lookup(si, elfhash(symbol), symbol);
    if (!s)
        return 0;
    sym_offset = s - si->symtab;
    rel = si->plt_rel;
    /* walk through reloc table, find symbol index matching one we've got */
    for (int i = 0; i < si->plt_rel_count; i++, rel++) {
        unsigned type = ELF32_R_TYPE(rel->r_info);
        unsigned sym = ELF32_R_SYM(rel->r_info);
        unsigned reloc = (unsigned)(rel->r_offset + si->base);
        unsigned oldval = 0;
        if (sym_offset == sym) {
            switch(type) {
                case R_ARM_JUMP_SLOT:
                    /* we do not have to read original value, but it would be good
                      idea to make sure it contains what we are looking for */
                    oldval = *(unsigned*) reloc;
                    *((unsigned*)reloc) = newval;
                    return 1;
                default:
                    return 0;
            }
        }
    }
    return 0;
}