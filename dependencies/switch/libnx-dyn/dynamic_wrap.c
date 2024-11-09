
#include <switch.h>
#include <elf.h>

void *__nx_aslr_base;

extern void __real___nx_dynamic(uintptr_t base, const Elf64_Dyn* dyn);

void __wrap___nx_dynamic(uintptr_t base, const Elf64_Dyn* dyn) {

    // Custom way to get base address to be used from modules later.
    __nx_aslr_base = (void*)base;

    // Normal __nx_dynamic
    __real___nx_dynamic(base, dyn);
}