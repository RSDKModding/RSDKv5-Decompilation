
namespace Legacy
{

#define LEGACY_GLOBALVAR_COUNT (0x100)

#define LEGACY_SAVEDATA_SIZE (0x2000)

#if RETRO_USE_MOD_LOADER
#define LEGACY_v4_NATIIVEFUNCTION_COUNT (0x30)
#else
#define LEGACY_v4_NATIIVEFUNCTION_COUNT (0x10)
#endif

struct GlobalVariable {
    char name[0x20];
    int32 value;
};

extern void *nativeFunction[LEGACY_v4_NATIIVEFUNCTION_COUNT];
extern int32 nativeFunctionCount;

extern int32 globalVariablesCount;
extern GlobalVariable globalVariables[LEGACY_GLOBALVAR_COUNT];

extern int32 saveRAM[LEGACY_SAVEDATA_SIZE];

int32 GetGlobalVariableByName(const char *name);
void SetGlobalVariableByName(const char *name, int value);
int32 GetGlobalVariableID(const char *name);

#define AddNativeFunction(name, funcPtr)                                                                                                             \
    if (nativeFunctionCount < LEGACY_v4_NATIIVEFUNCTION_COUNT) {                                                                                     \
        SetGlobalVariableByName(name, nativeFunctionCount);                                                                                          \
        nativeFunction[nativeFunctionCount++] = (void *)funcPtr;                                                                                     \
    }

bool32 ReadSaveRAM();
bool32 WriteSaveRAM();

namespace v4
{
// Native Functions
void NotifyCallback(int32 *callback, int32 *param1, int32 *param2, int32 *param3);

}


} // namespace Legacy