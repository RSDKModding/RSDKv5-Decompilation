
namespace Legacy
{

namespace v4
{

#define LEGACY_v4_SCRIPTCODE_COUNT (0x40000)
#define LEGACY_v4_JUMPTABLE_COUNT  (0x4000)
#define LEGACY_v4_FUNCTION_COUNT   (0x200)

#define LEGACY_v4_JUMPSTACK_COUNT (0x400)
#define LEGACY_v4_FUNCSTACK_COUNT (0x400)
#define LEGACY_v4_FORSTACK_COUNT  (0x400)

struct ScriptPtr {
    int32 scriptCodePtr;
    int32 jumpTablePtr;
};

struct ScriptFunction {
    uint8 access;
#if LEGACY_RETRO_USE_COMPILER
    char name[0x20];
#endif
    ScriptPtr ptr;
};

struct ObjectScript {
    int32 frameCount;
    int32 spriteSheetID;
    ScriptPtr eventUpdate;
    ScriptPtr eventDraw;
    ScriptPtr eventStartup;
    int32 frameListOffset;
    AnimationFile *animFile;
};

struct ScriptEngine {
    int32 operands[0x10];
    int32 temp[8];
    int32 arrayPosition[9];
    int32 checkResult;
};

enum ScriptSubs { EVENT_MAIN = 0, EVENT_DRAW = 1, EVENT_SETUP = 2 };

extern ObjectScript objectScriptList[LEGACY_v4_OBJECT_COUNT];
extern ScriptFunction scriptFunctionList[LEGACY_v4_FUNCTION_COUNT];

extern int32 scriptCode[LEGACY_v4_SCRIPTCODE_COUNT];
extern int32 jumpTable[LEGACY_v4_JUMPTABLE_COUNT];

extern int32 jumpTableStack[LEGACY_v4_JUMPSTACK_COUNT];
extern int32 functionStack[LEGACY_v4_FUNCSTACK_COUNT];
extern int32 foreachStack[LEGACY_v4_FORSTACK_COUNT];

extern int32 scriptCodePos;
extern int32 scriptCodeOffset;
extern int32 jumpTablePos;
extern int32 jumpTableOffset;
extern int32 jumpTableStackPos;
extern int32 functionStackPos;
extern int32 foreachStackPos;

extern ScriptEngine scriptEng;
extern char scriptText[0x4000];

#if LEGACY_RETRO_USE_COMPILER
extern int32 scriptFunctionCount;
extern char scriptFunctionNames[LEGACY_v4_FUNCTION_COUNT][0x40];

extern int32 lineID;

void CheckAliasText(char *text);
void CheckStaticText(char *text);
bool32 CheckTableText(char *text);
void ConvertArithmaticSyntax(char *text);
void ConvertConditionalStatement(char *text);
bool32 ConvertSwitchStatement(char *text);
void ConvertFunctionText(char *text);
void CheckCaseNumber(char *text);
bool32 ReadSwitchCase(char *text);
void ReadTableValues(char *text);
void AppendIntegerToString(char *text, int32 value);
void CopyAliasStr(char *dest, char *text, bool32 arrayIndex);
bool32 CheckOpcodeType(char *text); // Never actually used

void ParseScriptFile(char *scriptName, int32 scriptID);
#endif
void LoadBytecode(int32 scriptID, bool32 globalCode);

void ProcessScript(int32 scriptCodeStart, int32 jumpTableStart, uint8 scriptEvent);

void ClearScriptData();

} // namespace v4

} // namespace Legacy