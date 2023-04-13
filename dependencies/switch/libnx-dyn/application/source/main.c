#include <stdlib.h>
#include <stdio.h>
#include <dyn.h>

typedef struct
{
    u32(*DummyFunction)();
} ModuleSampleStruct;

u64 getBaseAddress()
{
    u32 p;
    MemoryInfo info;
    svcQueryMemory(&info, &p, (u64)getBaseAddress); // Query the memory region of a function to get out base address
    return info.addr;
}

int main(int argc, char* argv[])
{
    consoleInit(NULL);
    Result rc = romfsInit();

    if(R_FAILED(rc)) {
        printf("romfsInit() failed: 0x%x\n", rc);
    }

    if(R_SUCCEEDED(rc)) {
        rc = dynInitialize();
        if(R_FAILED(rc)) {
            printf("dynInitialize() failed: 0x%x\n", rc);
        }

        if(R_SUCCEEDED(rc)) {
            // Got from our __nx_dynamic wrap
            extern void *__nx_aslr_base;
            // Got from SVC
            u64 addr = getBaseAddress();

            // addr and __nx_aslr_base must be equal!
            printf ("Addr1: %p -> Addr2: %p (equal? %d)\n\n", __nx_aslr_base, (void*)addr, (__nx_aslr_base == (void*)addr));

            // Load ourselves with our base address
            rc = dynLoadFromMemory("main", __nx_aslr_base);
            if(R_FAILED(rc)) {
                printf("dynLoadFromMemory() failed: 0x%x\n", rc);
            }

            if(R_SUCCEEDED(rc)) {
                printf("Address of self module 'main': %p\n", __nx_aslr_base);
                DynModule sample_mod;
                memset(&sample_mod, 0, sizeof(sample_mod));

                rc = dynLoadNroModule(&sample_mod, "romfs:/libmodule.nro", false);
                if(R_FAILED(rc)) {
                    printf("dynLoadNroModule() failed: 0x%x\n", rc);
                }

                if(R_SUCCEEDED(rc)) {
                    printf("Address of loaded module 'libmodule.nro': %p\n", sample_mod.input.base);
                    void *symbol = NULL;
                    rc = dynModuleLookupSymbol(&sample_mod, "libmodule_setConfig", &symbol);
                    if(R_FAILED(rc)) {
                        printf("dynModuleLookupSymbol() failed: 0x%x\n", rc);
                    }

                    if(R_SUCCEEDED(rc)) {
                        printf("Symbol 'libmodule_setConfig()' address: %p\n", symbol);
                        if(symbol) {
                            void(*func)(ModuleSampleStruct*) = (void(*)(ModuleSampleStruct*))symbol;
                            ModuleSampleStruct sample;
                            sample.DummyFunction = NULL;
                            func(&sample); // Calling module's function to set its own function in the struct
                            if(sample.DummyFunction) {
                                u32 val = sample.DummyFunction();
                                printf("Got return value %d from our module's 'DummyFunction()'!\n", val);
                            }
                        }
                    }
                }
            }
        }
    }

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    dynUnloadAll();
    dynExit();

    romfsExit();
    consoleExit(NULL);

    return 0;
}