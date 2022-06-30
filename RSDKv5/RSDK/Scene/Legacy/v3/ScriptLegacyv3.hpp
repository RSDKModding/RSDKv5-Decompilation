
namespace Legacy
{

namespace v3
{

#define LEGACY_v3_SCRIPTDATA_COUNT (0x40000)
#define LEGACY_v3_JUMPTABLE_COUNT (0x4000)
#define LEGACY_v3_FUNCTION_COUNT (0x200)

#define LEGACY_v3_JUMPSTACK_COUNT (0x400)
#define LEGACY_v3_FUNCSTACK_COUNT (0x400)

struct ScriptPtr {
    int32 scriptCodePtr;
    int32 jumpTablePtr;
};

struct ScriptFunction {
#if LEGACY_RETRO_USE_COMPILER
    char name[0x20];
#endif
    ScriptPtr ptr;
};

struct ObjectScript {
    int32 frameCount;
    int32 spriteSheetID;
    ScriptPtr subMain;
    ScriptPtr subPlayerInteraction;
    ScriptPtr subDraw;
    ScriptPtr subStartup;
    int32 frameListOffset;
    AnimationFile *animFile;
};

struct ScriptEngine {
    int32 operands[10];
    int32 tempValue[8];
    int32 arrayPosition[3];
    int32 checkResult;
};

enum ScriptSubs { SUB_MAIN = 0, SUB_PLAYERINTERACTION = 1, SUB_DRAW = 2, SUB_SETUP = 3 };

extern ObjectScript objectScriptList[LEGACY_v3_OBJECT_COUNT];

extern ScriptFunction scriptFunctionList[LEGACY_v3_FUNCTION_COUNT];
extern int32 scriptFunctionCount;

extern int32 scriptCode[LEGACY_v3_SCRIPTDATA_COUNT];
extern int32 jumpTable[LEGACY_v3_JUMPTABLE_COUNT];

extern int32 jumpTableStack[LEGACY_v3_JUMPSTACK_COUNT];
extern int32 functionStack[LEGACY_v3_FUNCSTACK_COUNT];

extern int32 scriptCodePos;
extern int32 scriptCodeOffset;
extern int32 jumpTablePos;
extern int32 jumpTableOffset;

extern int32 jumpTableStackPos;
extern int32 functionStackPos;

extern ScriptEngine scriptEng;
extern char scriptText[0x100];

extern int32 aliasCount;
extern int32 lineID;

#if LEGACY_RETRO_USE_COMPILER

void CheckAliasText(char *text);
void ConvertArithmaticSyntax(char *text);
void ConvertIfWhileStatement(char *text);
bool32 ConvertSwitchStatement(char *text);
void ConvertFunctionText(char *text);
void CheckCaseNumber(char *text);
bool32 ReadSwitchCase(char *text);
void AppendIntegerToString(char *text, int32 value);
void CopyAliasStr(char *dest, char *text, bool32 arrayIndex);
bool32 CheckOpcodeType(char *text); // Never actually used

void ParseScriptFile(char *scriptName, int32 scriptID);
#endif

void LoadBytecode(int32 scriptID, bool32 globalCode);

void ProcessScript(int32 scriptCodeStart, int32 jumpTableStart, byte scriptSub);

void ClearScriptData();

}

} // namespace Legacy