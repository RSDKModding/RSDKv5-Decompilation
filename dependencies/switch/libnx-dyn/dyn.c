#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <dyn.h>
#include <address_space.h>

static DynModule *g_ModuleList[MAX_MODULES];
static u32 g_ModuleCount = 0;

#define DBGFMT(fmt, ...)                                                                                                                             \
    {                                                                                                                                                \
        printf(fmt "\n", ##__VA_ARGS__);                                                                                                             \
        consoleUpdate(NULL);                                                                                                                         \
    }

Result dynElfFindValue(Elf64_Dyn *dynamic, s64 tag, u64 *value)
{
    u64 *found = NULL;
    *value     = 0;
    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        if (dynamic->d_tag == tag) {
            if (found != NULL)
                return MAKERESULT(Module_LibnxDyn, LibnxDynError_DuplicatedDtEntry);
            else
                found = &dynamic->d_un.d_val;
        }
    }
    if (found == NULL)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry);
    *value = *found;
    return 0;
}

Result dynElfFindOffset(Elf64_Dyn *dynamic, s64 tag, void **value, void *aslr_base)
{
    u64 intermediate;
    Result r = dynElfFindValue(dynamic, tag, &intermediate);
    *value   = (u8 *)aslr_base + intermediate;
    return r;
}

u64 dynElfHashString(const char *name)
{
    u64 h = 0;
    u64 g;
    while (*name) {
        h = (h << 4) + *(const u8 *)name++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

static Handle ownHandle;

static Result _dynLoad_elf(const char *path, DynModule *mod)
{
    Result res = MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputElf);
    FILE *f    = fopen(path, "rb");
    u64 appid  = 0x010000000000100D;
    svcGetInfo(&appid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);

    if (f) {
        fseek(f, 0, SEEK_END);
        size_t filesz = ftell(f);

        rewind(f);
        void *file = memalign(0x1000, filesz);
        if (!file) {
            fclose(f);
            return res;
        }

        size_t read = fread(file, 1, filesz, f);
        if (read != filesz) {
            free(file);
            fclose(f);
            return res;
        }
        fclose(f);

        Dyn_Elf64_Data *data = malloc(sizeof(*data));
        if (data == NULL) {
            return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        }
        data->num_segments = 0;

        Elf64_Ehdr *ehdr  = (Elf64_Ehdr *)file;
        Elf64_Phdr *phdrs = file + ehdr->e_phoff;
        if (ehdr->e_phoff + sizeof(Elf64_Phdr) * ehdr->e_phnum > filesz) {
            return res;
        }
        uint64_t total_as_size = 0;
        for (int i = 0; i < ehdr->e_phnum; i++) {
            Elf64_Phdr *phdr = &phdrs[i];
            if (phdr->p_type == PT_LOAD && phdr->p_memsz > 0) {
                data->num_segments++;
            }
            if (phdr->p_vaddr + phdr->p_memsz > total_as_size) {
                total_as_size = phdr->p_vaddr + phdr->p_memsz;
            }
        }

        data->segments = malloc(sizeof(*data->segments) * data->num_segments);
        if (data->segments == NULL) {
            return res;
        }

        // in theory this is safe :rapture:
        void *slide   = as_reserve(total_as_size);
        data->as_base = slide;
        data->as_size = total_as_size;

        for (int i = 0, j = 0; i < ehdr->e_phnum; i++) {
            Elf64_Phdr *phdr = &phdrs[i];
            if (phdr->p_type == PT_LOAD && phdr->p_memsz > 0) {
                Dyn_Elf64_Seg *seg = &data->segments[j++];
                seg->phdr          = *phdr;
                seg->size          = phdr->p_memsz;
                seg->dst           = slide + phdr->p_vaddr;
                seg->src           = file + phdr->p_offset;

                if (phdr->p_offset + phdr->p_filesz > filesz) {
                    for (int k = 0; k < j - 1; k++) {
                        if (data->segments[k].clone) {
                            free(data->segments[k].clone);
                        }
                    }
                    return res;
                }

                if (phdr->p_filesz == phdr->p_memsz && (phdr->p_memsz & 0xFFF) == 0 && (phdr->p_offset & 0xFFF) == 0) {
                    seg->clone = NULL;
                }
                else {
                    seg->size  = (phdr->p_memsz + 0xFFF) & ~0xFFF;
                    seg->clone = memalign(0x1000, seg->size);
                    if (seg->clone == NULL) {
                        for (int k = 0; k < j - 1; k++) {
                            if (data->segments[k].clone) {
                                free(data->segments[k].clone);
                            }
                        }
                        res = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
                        return res;
                    }
                    memset(seg->clone + phdr->p_filesz, 0, seg->size - phdr->p_filesz);
                    memcpy(seg->clone, seg->src, phdr->p_filesz);
                    seg->src = seg->clone;
                }
            }
        }

        for (uint64_t i = 0; i < data->num_segments; i++) {
            Dyn_Elf64_Seg *seg = &data->segments[i];
            if (R_FAILED(res = svcMapProcessCodeMemory(ownHandle, seg->dst, seg->src, seg->size))) {
                res = 0;
                /*for (uint64_t j = 0; j < i; j++) {
                    svcUnmapProcessCodeMemory(ownHandle, seg->dst, seg->src, seg->size);
                }
                return res;//*/
            }

            uint32_t permissions = 0;
            if (seg->phdr.p_flags & PF_X) {
                permissions |= 4;
            }
            if (seg->phdr.p_flags & PF_W) {
                permissions |= 2;
            }
            if (seg->phdr.p_flags & PF_R) {
                permissions |= 1;
            }

            if (R_FAILED(res = svcSetProcessMemoryPermission(ownHandle, (uint64_t)seg->dst, seg->size, permissions))) {
                res = 0;
                /*for (uint64_t j = 0; j <= i; j++) {
                    svcUnmapProcessCodeMemory(ownHandle, seg->dst, seg->src, seg->size);
                }
                return res;//*/
            }
        }
        mod->input.base        = slide;
        mod->input.loader_data = data;
    }
    return res;
}

static Result _dynLoad_nrovsc(const char *path, DynModule *mod)
{
    Result res = MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);
    FILE *f    = fopen(path, "rb");
    if (f) {
        fseek(f, sizeof(NroStart), SEEK_SET);
        NroHeader header;
        fread(&header, 1, sizeof(NroHeader), f);
        size_t filesz = header.size;

        rewind(f);
        void *nro = memalign(0x1000, filesz);
        if (!nro) {
            fclose(f);
            return res;
        }

        size_t read = fread(nro, 1, filesz, f);
        if (read != filesz) {
            free(nro);
            fclose(f);
            return res;
        }
        fclose(f);

        u32 *nrr = (u32 *)memalign(0x1000, 0x1000);
        if (!nrr) {
            free(nro);
            return res;
        }
        memset(nrr, 0, 0x1000);

        nrr[0]                = NRR0_MAGIC;
        nrr[(0x338 >> 2) + 0] = 0x1000;
        nrr[(0x340 >> 2) + 0] = 0x350;
        nrr[(0x340 >> 2) + 1] = 1; // NRO count

        u64 appid = 0x010000000000100D;
        svcGetInfo(&appid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);

        *(u64 *)&((u8 *)nrr)[0x330] = appid;

        sha256CalculateHash(&nrr[0x350 >> 2], nro, filesz);

        u32 bss_sz = *(u32 *)((u8 *)nro + 0x38);
        void *bss  = memalign(0x1000, bss_sz);
        if (!bss) {
            free(nro);
            free(nrr);
            return res;
        }

        u64 nro_addr = 0;

        res = ldrRoLoadNrr((u64)nrr, 0x1000);
        if (res == 0)
            res = ldrRoLoadNro(&nro_addr, (u64)nro, filesz, (u64)bss, bss_sz);

        if ((res == 0)) {
            mod->input.nro  = nro;
            mod->input.nrr  = nrr;
            mod->input.bss  = bss;
            mod->input.base = nro_addr;
        }
    }
    return res;
}

static Result _dynRelocate(DynModule *mod);
static Result _dynInitialize(DynModule *mod);

static Result _dynLoad(const char *path, DynModule *mod)
{
    if (R_FAILED(_dynLoad_nrovsc(path, mod))) {
        return _dynLoad_elf(path, mod);
    }
}

static Result _dynScan(DynModule *mod)
{
    u8 *module_base             = (u8 *)mod->input.base;
    u32 mod0_offset             = *(u32 *)&(module_base)[4];
    DynModuleHeader *mod_header = (DynModuleHeader *)&module_base[mod0_offset];
    Elf64_Dyn *dynamic          = (Elf64_Dyn *)((u8 *)mod_header + mod_header->dynamic);
    mod->dynamic                = dynamic;

    if (mod_header->magic != MOD0_MAGIC)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);

    Result r = dynElfFindOffset(dynamic, DT_HASH, &mod->hash, module_base);
    if ((r != 0) && (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)))
        return r;

    r = dynElfFindOffset(dynamic, DT_STRTAB, &mod->strtab, module_base);
    if ((r != 0) && (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)))
        return r;

    r = dynElfFindOffset(mod->dynamic, DT_SYMTAB, &mod->symtab, module_base);
    if ((r != 0) && (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)))
        return r;

    u64 syment;
    r = dynElfFindValue(mod->dynamic, DT_SYMENT, &syment);
    if ((r == 0)) {
        if (syment != sizeof(Elf64_Sym))
            return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidSymEnt);
    }
    else if (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry))
        return r;

    for (Elf64_Dyn *walker = dynamic; walker->d_tag != DT_NULL; walker++) {
        if (walker->d_tag == DT_NEEDED) {
            DynModule *dep = malloc(sizeof(DynModule));
            r              = _dynLoad(mod->strtab + walker->d_un.d_val, dep);
            if ((r == 0)) {
                mod->dependencies[mod->dependency_count] = dep;
                mod->dependency_count++;
            }
        }
    }

    mod->state = DynModuleState_Scanned;
    return 0;
}

static Result _dynTryResolveSymbol(DynModule *try_mod, const char *find_name, u64 find_name_hash, Elf64_Sym **def, DynModule **defining_module,
                                   bool require_global)
{
    if (require_global && !try_mod->input.is_global)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
    if (try_mod->symtab == NULL)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
    if (try_mod->strtab == NULL)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
    if (try_mod->hash != NULL) {
        u32 nbucket = try_mod->hash[0];
        u32 nchain  = try_mod->hash[1];
        (void)nchain;
        u32 index   = try_mod->hash[2 + (find_name_hash % nbucket)];
        u32 *chains = try_mod->hash + 2 + nbucket;
        while (index != 0 && strcmp(find_name, try_mod->strtab + try_mod->symtab[index].st_name) != 0) {
            index = chains[index];
        }
        if (index == STN_UNDEF) {
            return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
        }
        Elf64_Sym *sym = &try_mod->symtab[index];
        if (sym->st_shndx == SHN_UNDEF) {
            return 0x100;
            MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
        }
        *def             = sym;
        *defining_module = try_mod;
        return 0;
    }
    return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
}

Result _dynResolveLoadSymbol(DynModule *find_mod, const char *find_name, Elf64_Sym **def, DynModule **defining_module)
{
    u64 hash = dynElfHashString(find_name);

    for (u32 i = 0; i < g_ModuleCount; i++) {
        if (g_ModuleList[i] != find_mod) {
            Result res = _dynTryResolveSymbol(g_ModuleList[i], find_name, hash, def, defining_module, true);
            if (res == MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol)) {
                continue;
            }
            else {
                return res;
            }
        }
    }

    if (find_mod != NULL) {
        return _dynTryResolveSymbol(find_mod, find_name, hash, def, defining_module, false);
    }

    return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
}

Result _dynResolveDependencySymbol(DynModule *find_mod, const char *find_name, Elf64_Sym **def, DynModule **defining_module)
{
    u64 find_name_hash = dynElfHashString(find_name);
    Result r           = _dynTryResolveSymbol(find_mod, find_name, find_name_hash, def, defining_module, false);
    if (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol))
        return r;

    for (u32 i = 0; i < find_mod->dependency_count; i++) {
        r = _dynTryResolveSymbol(find_mod->dependencies[i], find_name, find_name_hash, def, defining_module, false);
        if (r == MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol)) {
            continue;
        }
        else {
            return r;
        }
    }

    for (u32 i = 0; i < find_mod->dependency_count; i++) {
        r = _dynResolveDependencySymbol(find_mod->dependencies[i], find_name, def, defining_module);
        if (r == MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol)) {
            continue;
        }
        else {
            return r;
        }
    }

    return MAKERESULT(Module_LibnxDyn, LibnxDynError_CouldNotResolveSymbol);
}

Result _dynRelocateModuleBase(u8 *module_base)
{
    DynModuleHeader *mod_header = (DynModuleHeader *)&module_base[*(u32 *)&module_base[4]];
    Elf64_Dyn *dynamic          = (Elf64_Dyn *)(((u8 *)mod_header) + mod_header->dynamic);
    u64 rela_offset             = 0;
    u64 rela_size               = 0;
    u64 rela_ent                = 0;
    u64 rela_count              = 0;

    if (mod_header->magic != MOD0_MAGIC) {
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);
    }

    Result r = dynElfFindValue(dynamic, DT_RELA, &rela_offset);
    if (R_FAILED(r)) {
        return r;
    }

    r = dynElfFindValue(dynamic, DT_RELASZ, &rela_size);
    if (R_FAILED(r)) {
        return r;
    }

    r = dynElfFindValue(dynamic, DT_RELAENT, &rela_ent);
    if (R_FAILED(r)) {
        return r;
    }

    r = dynElfFindValue(dynamic, DT_RELACOUNT, &rela_count);
    if (R_FAILED(r)) {
        return r;
    }

    if (rela_ent != 0x18) {
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocEnt);
    }

    if (rela_size != rela_count * rela_ent) {
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocTableSize);
    }

    Dyn_Elf64_Rela *rela_base = (Dyn_Elf64_Rela *)(module_base + rela_offset);
    for (u64 i = 0; i < rela_count; i++) {
        Dyn_Elf64_Rela rela = rela_base[i];

        switch (rela.r_reloc_type) {
            case 0x403:
                if (rela.r_symbol != 0) {
                    return MAKERESULT(Module_LibnxDyn, LibnxDynError_RelaUnsupportedSymbol);
                }
                *(void **)(module_base + rela.r_offset) = module_base + rela.r_addend;
                break;
            default: return MAKERESULT(Module_LibnxDyn, LibnxDynError_UnrecognizedRelocType);
        }
    }

    return 0;
}

static Result _dynRunRelocationTable(DynModule *mod, u32 offset_tag, u32 size_tag)
{
    void *raw_table;
    Elf64_Dyn *dynamic = mod->dynamic;
    Result r           = dynElfFindOffset(dynamic, offset_tag, &raw_table, mod->input.base);
    if (r == MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)) {
        return 0;
    }
    if (R_FAILED(r)) {
        return r;
    }

    u64 table_size = 0;
    u64 table_type = offset_tag;
    r              = dynElfFindValue(dynamic, size_tag, &table_size);
    if (R_FAILED(r)) {
        return r;
    }

    if (offset_tag == DT_JMPREL) {
        r = dynElfFindValue(dynamic, DT_PLTREL, &table_type);
        if (R_FAILED(r)) {
            return r;
        }
    }

    u64 ent_size = 0;
    switch (table_type) {
        case DT_RELA:
            r = dynElfFindValue(dynamic, DT_RELAENT, &ent_size);
            if (r == MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)) {
                ent_size = sizeof(Elf64_Rela);
            }
            else if (r == 0 && ent_size != sizeof(Elf64_Rela)) {
                return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocEnt);
            }
            else if (r != 0) {
                return r;
            }
            break;
        case DT_REL:
            r = dynElfFindValue(dynamic, DT_RELENT, &ent_size);
            if (r == MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry)) {
                ent_size = sizeof(Elf64_Rel);
            }
            else if (r == 0 && ent_size != sizeof(Elf64_Rel)) {
                return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocEnt);
            }
            else if (r != 0) {
                return r;
            }
            break;
        default: return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocTableType);
    }

    if ((table_size % ent_size) != 0)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidRelocTableSize);

    for (size_t offset = 0; offset < table_size; offset += ent_size) {
        Dyn_Elf64_Rela rela;
        switch (table_type) {
            case DT_RELA: rela = *(Dyn_Elf64_Rela *)((u8 *)raw_table + offset); break;
            case DT_REL: {
                Dyn_Elf64_Rel rel = *(Dyn_Elf64_Rel *)((u8 *)raw_table + offset);
                rela.r_offset     = rel.r_offset;
                rela.r_reloc_type = rel.r_reloc_type;
                rela.r_symbol     = rel.r_symbol;
                break;
            }
        }

        void *symbol               = NULL;
        DynModule *defining_module = mod;
        if (rela.r_symbol != 0) {
            if (mod->symtab == NULL)
                return MAKERESULT(Module_LibnxDyn, LibnxDynError_NeedsSymTab);
            if (mod->strtab == NULL)
                return MAKERESULT(Module_LibnxDyn, LibnxDynError_NeedsStrTab);
            Elf64_Sym *sym = &mod->symtab[rela.r_symbol];

            Elf64_Sym *def;
            r = _dynResolveLoadSymbol(mod, mod->strtab + sym->st_name, &def, &defining_module);
            if ((r != 0))
                return r;
            symbol = (u8 *)defining_module->input.base + def->st_value;
        }
        void *delta_symbol = defining_module->input.base;

        switch (rela.r_reloc_type) {
            case 257:
            case 1025:
            case 1026: {
                void **target = (void **)((u8 *)mod->input.base + rela.r_offset);
                if (table_type == DT_REL)
                    rela.r_addend = (u64)*target;
                *target = (u8 *)symbol + rela.r_addend;
                break;
            }
            case 1027: {
                if (!mod->input.has_run_basic_relocations) {
                    void **target = (void **)((u8 *)mod->input.base + rela.r_offset);
                    if (table_type == DT_REL)
                        rela.r_addend = (u64)*target;
                    *target = (u8 *)delta_symbol + rela.r_addend;
                }
                break;
            }
            default: return MAKERESULT(Module_LibnxDyn, LibnxDynError_UnrecognizedRelocType);
        }
    }

    return 0;
}

static Result _dynRelocate(DynModule *mod)
{
    Result r = _dynRunRelocationTable(mod, DT_RELA, DT_RELASZ);
    if ((r != 0))
        return r;
    r = _dynRunRelocationTable(mod, DT_REL, DT_RELSZ);
    if ((r != 0))
        return r;
    r          = _dynRunRelocationTable(mod, DT_JMPREL, DT_PLTRELSZ);
    mod->state = DynModuleState_Relocated;
    return r;
}

static Result _dynInitialize(DynModule *mod)
{
    if (mod->state != DynModuleState_Relocated)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidModuleState);

    void (**init_array)(void);
    size_t init_array_size;

    Result r = dynElfFindOffset(mod->dynamic, DT_INIT_ARRAY, (void **)&init_array, mod->input.base);
    if ((r == 0)) {
        r = dynElfFindValue(mod->dynamic, DT_INIT_ARRAYSZ, &init_array_size);
        if ((r != 0))
            return r;
        for (size_t i = 0; i < (init_array_size / sizeof(init_array[0])); i++) init_array[i]();
    }
    else if (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry))
        return r;
    mod->state = DynModuleState_Initialized;
    return 0;
}

static Result _dynFinalize(DynModule *mod)
{
    if (mod->state != DynModuleState_Initialized)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidModuleState);

    void (**fini_array)(void);
    size_t fini_array_size;

    Result r = dynElfFindOffset(mod->dynamic, DT_FINI_ARRAY, (void **)&fini_array, mod->input.base);
    if ((r == 0)) {
        r = dynElfFindValue(mod->dynamic, DT_FINI_ARRAYSZ, &fini_array_size);
        if ((r != 0))
            return r;
        for (size_t i = 0; i < (fini_array_size / sizeof(fini_array[0])); i++) fini_array[i]();
    }
    else if (r != MAKERESULT(Module_LibnxDyn, LibnxDynError_MissingDtEntry))
        return r;

    mod->state = DynModuleState_Finalized;
    return 0;
}

static Result _dynDestroy(DynModule *mod);

static void _dynDecref(DynModule *mod)
{
    mod->ref_count--;
    if (mod->ref_count == 0)
        _dynDestroy(mod);
}

static Result _dynUnload(DynModule *mod)
{
    Result res;
    if (mod != NULL) {
        if (mod->input.loader_data == NULL) {
            free(mod->input.nrr);
            free(mod->input.nro);
            free(mod->input.bss);
            res = ldrRoUnloadNro((u64)mod->input.base);
            if ((res == 0))
                res = ldrRoUnloadNrr((u64)mod->input.nrr);
        }
        else {
            Dyn_Elf64_Data *data = mod->input.loader_data;
            for (uint64_t i = 0; i < data->num_segments; i++) {
                Dyn_Elf64_Seg *seg = &data->segments[i];
                svcUnmapProcessCodeMemory(ownHandle, seg->dst, seg->src, seg->size);
                if (seg->clone) {
                    free(seg->clone);
                }
            }

            as_release(data->as_base, data->as_size);
            free(data->segments);
            free(data);
        }
    }
    return res;
}

static Result _dynDestroy(DynModule *mod)
{
    Result res;
    if (mod->state == DynModuleState_Initialized) {
        res = _dynFinalize(mod);
        if ((res == 0))
            res = _dynUnload(mod);
    }

    for (u32 i = 0; i < mod->dependency_count; i++) _dynDecref(mod->dependencies[i]);

    return res;
}

static void _dynDestroyModule(DynModule *mod) { _dynDecref(mod); }

static void _dynEraseFromModules(u32 index)
{
    if (index >= g_ModuleCount)
        return;
    u32 i;
    for (i = index + 1; i < g_ModuleCount; i++) {
        g_ModuleList[i - 1] = g_ModuleList[i];
        g_ModuleList[i]     = NULL;
    }
    g_ModuleCount--;
}

Result dynInitialize()
{
    ownHandle = envGetOwnProcessHandle();
    if (!envIsSyscallHinted(0x77) || !envIsSyscallHinted(0x78))
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InsufficientSysCalls);
    return ldrRoInitialize();
}

void dynExit() { ldrRoExit(); }

Result dynLoadFromMemory(const char *name, void *base)
{
    if (!name)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);
    if (!base)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);

    DynModule *mod = malloc(sizeof(DynModule));
    if (!mod)
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);

    memset(mod, 0, sizeof(DynModule));
    strcpy(mod->input.name, name);
    mod->input.has_run_basic_relocations = true;
    mod->input.is_global                 = true;
    mod->input.base                      = base;
    mod->input.is_nro                    = false;
    mod->dependency_count                = 0;

    mod->ref_count = 1;
    mod->state     = DynModuleState_Queued;

    Result res = _dynScan(mod);
    if (res != 0) {
        free(mod);
        return res;
    }

    res = _dynRelocate(mod);
    if (res != 0) {
        free(mod);
        return res;
    }

    res = _dynInitialize(mod);
    if (res != 0) {
        free(mod);
        return res;
    }

    g_ModuleList[g_ModuleCount] = mod;
    g_ModuleCount++;

    return res;
}

Result dynLoadNroModule(DynModule *mod, const char *path, bool global)
{
    if (path == NULL) {
        return MAKERESULT(Module_LibnxDyn, LibnxDynError_InvalidInputNro);
    }

    strcpy(mod->input.name, path);
    mod->input.has_run_basic_relocations = false;
    mod->input.is_global                 = global;
    mod->dependency_count                = 0;

    Result res = _dynLoad(path, mod);

    if (R_FAILED(res)) {
        return res;
    }

    mod->ref_count = 1;
    mod->state     = DynModuleState_Queued;

    res = _dynScan(mod);
    if (R_FAILED(res)) {
        return res;
    }

    res = _dynRelocate(mod);
    if (R_FAILED(res)) {
        return res;
    }

    res = _dynInitialize(mod);
    if (R_FAILED(res)) {
        return res;
    }

    g_ModuleList[g_ModuleCount] = mod;
    g_ModuleCount++;

    return res;
}

void dynModuleUnload(DynModule *mod) { _dynDestroyModule(mod); }

void dynUnloadMyName(const char *name)
{
    for (u32 i = 0; i < g_ModuleCount; i++) {
        if (strcmp(name, g_ModuleList[i]->input.name) == 0) {
            _dynDestroyModule(g_ModuleList[i]);
            _dynEraseFromModules(i);
            break;
        }
    }
}

void dynUnloadAll()
{
    for (u32 i = 0; i < g_ModuleCount; i++) {
        _dynDestroyModule(g_ModuleList[i]);
        g_ModuleList[i] = NULL;
    }
    g_ModuleCount = 0;
}

Result dynModuleLookupSymbol(DynModule *mod, const char *name, void **out_sym)
{
    Elf64_Sym *def;
    DynModule *def_mod;

    Result res = _dynResolveDependencySymbol(mod, name, &def, &def_mod);
    if (R_SUCCEEDED(res)) {
        *out_sym = (u8 *)mod->input.base + def->st_value;
    }
    return res;
}
