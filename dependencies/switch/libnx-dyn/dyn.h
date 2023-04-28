
#pragma once
#include <elf.h>
#include <switch.h>

#define NRR0_MAGIC 0x3052524E
#define MOD0_MAGIC 0x30444F4D

typedef struct
{
    u32 magic;
    u32 dynamic;
    u32 bss_start;
    u32 bss_end;
    u32 unwind_start;
    u32 unwind_end;
    u32 module_object;
} DynModuleHeader;

typedef enum
{
    DynModuleState_Invalid,
    DynModuleState_Queued,
    DynModuleState_Scanned,
    DynModuleState_Relocated,
    DynModuleState_Initialized,
    DynModuleState_Finalized,
    DynModuleState_Unloaded
} DynModuleState;


typedef struct {
    char name[FS_MAX_PATH];
    void *base;
    void *nro;
    void *nrr;
    void *bss;
    void *loader_data;
    bool is_nro;
    bool is_global;
    bool has_run_basic_relocations;
} DynModuleInput;

#define MAX_MODULES 32 // This might be increased...
#define MAX_DEPENDENCIES 32

typedef struct DynModule DynModule;

struct DynModule {
    DynModuleState state;
    int ref_count;
    DynModuleInput input;
    DynModule *dependencies[MAX_DEPENDENCIES];
    u32 dependency_count;
    void *address;
    Elf64_Dyn *dynamic;
    Elf64_Sym *symtab;
    const char *strtab;
    u32 *hash;
};

enum {
    Module_LibnxDyn = 350,
};

enum {
    LibnxDynError_InvalidInputNro = 1,
    LibnxDynError_MissingDtEntry,
    LibnxDynError_DuplicatedDtEntry,
    LibnxDynError_InvalidSymEnt,
    LibnxDynError_InvalidModuleState,
    LibnxDynError_InvalidRelocEnt,
    LibnxDynError_InvalidRelocTableSize,
    LibnxDynError_RelaUnsupportedSymbol,
    LibnxDynError_UnrecognizedRelocType,
    LibnxDynError_InvalidRelocTableType,
    LibnxDynError_NeedsSymTab,
    LibnxDynError_NeedsStrTab,
    LibnxDynError_CouldNotResolveSymbol,

    LibnxDynError_InvalidInputElf = 20,
    LibnxDynError_InsufficientSysCalls,
};

// Custom ELF structs grabbed from libtransistor

typedef struct {
    u64 r_offset;
    u32 r_reloc_type;
    u32 r_symbol;
    u64 r_addend;
} Dyn_Elf64_Rela;

typedef struct {
    u64 r_offset;
    u32 r_reloc_type;
    u32 r_symbol;
} Dyn_Elf64_Rel;

typedef struct {
	Elf64_Phdr phdr;
	void *dst;
	void *src;
	void *clone;
	size_t size;
} Dyn_Elf64_Seg;

typedef struct {
	void *as_base;
	size_t as_size;
	Dyn_Elf64_Seg *segments;
	size_t num_segments;
} Dyn_Elf64_Data;

Result dynElfFindValue(Elf64_Dyn *dynamic, s64 tag, u64 *value);
Result dynElfFindOffset(Elf64_Dyn *dynamic, s64 tag, void **value, void *aslr_base);
u64 dynElfHashString(const char *str);

Result dynInitialize();
void dynExit();

Result dynLoadFromMemory(const char *name, void *addr);
Result dynLoadNroModule(DynModule *out, const char *path, bool global);
void dynModuleUnload(DynModule *mod);
void dynUnloadMyName(const char *name);
void dynUnloadAll();

Result dynModuleLookupSymbol(DynModule *mod, const char *name, void **out_sym);