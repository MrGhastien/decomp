#define _GNU_SOURCE
#include "defines.h"
#include "test.h"
#include "darray.h"

#include <dlfcn.h>
#include <link.h>
#include <string.h>
#include <stdio.h>

typedef struct AssertFail {
    const char* expression;
    const char* message;
    const char* filename;
    int line;
} AssertFail;

static AssertFail* failures;

int runTest(test_function function, const char* name) {
    darrayClear(failures);
    printf("\e[32;01m======= TEST %s =======\e[0m\n", name);
    function();
    if (darrayLength(failures) > 0) {
        fprintf(stderr, "Test has failed ! (%zu failures)\n", darrayLength(failures));
    }
    printf("\e[32;01m======= %s =======\e[0m\n", "END OF TEST");
    return 0;
}

bool endsWith(const char* str, const char* end) {
    size_t lenStr = strlen(str);
    size_t lenEnd = strlen(end);

    if(lenStr < lenEnd)
        return FALSE;

    size_t j = lenStr - 1;
    size_t i = lenEnd - 1;
    //This is equivalent to checking if i & j are above 0, but because
    // i & j are unsigned, they can't be negative.
    while(i < lenEnd && j < lenStr) {
        if(str[j] != end[i])
            return FALSE;
        j--;
        i--;
    }
    return i >= lenEnd;
}

int performTests(const char* programName) {

    failures = darrayCreate(4, sizeof *failures);

    printf("Entering test mode.%s", "\n");
    
    char* baseName = basename(programName);
    char testLibName[100];
    sprintf(testLibName, "./test-%s.so", baseName);
    void* testLibHandle = dlopen(testLibName, RTLD_LAZY | RTLD_GLOBAL);

    struct link_map* map = NULL;
    dlinfo(testLibHandle, RTLD_DI_LINKMAP, &map);
    
    Elf64_Sym* symtab;
    char* strtab;
    int entryCount = 0;

    for (ElfW(Dyn)* section = map->l_ld; section->d_tag != DT_NULL; section++) {
        if (section->d_tag == DT_SYMTAB) {
            symtab = (Elf64_Sym*)section->d_un.d_ptr;
        }
        if (section->d_tag == DT_STRTAB) {
            strtab = (char*)section->d_un.d_ptr;
        }
        if (section->d_tag == DT_SYMENT) {
            entryCount = section->d_un.d_val;
        }
    }
    int size = strtab - (char*)symtab;
    for (int k = 0; k < size / entryCount; k++) {
        Elf64_Sym* sym = &symtab[k];
        if (ELF64_ST_TYPE(symtab[k].st_info) == STT_FUNC) {
            char* str = &strtab[sym->st_name];
            if (endsWith(str, "_test")) {
                printf("Found test function %s !\n", str);
                test_function func = dlsym(testLibHandle, str);
                if (func != NULL)
                    runTest(func, str);
            }
        }
    }
    return 0;
}

void reportAssertFail(const char* expression, const char* message, const char* filename, int line) {
    AssertFail failure = {expression, message, filename, line};
    darrayAdd(&failures, failure);
    fprintf(stderr, "Assertion '%s' failed : %s (%s:%i)\n", expression, message, filename, line);
}
