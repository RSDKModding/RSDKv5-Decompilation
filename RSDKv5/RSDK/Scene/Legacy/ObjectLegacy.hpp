
#define LEGACY_RETRO_USE_COMPILER (1)

#include "v3/ObjectLegacyv3.hpp"
#include "v3/PlayerLegacyv3.hpp"
#include "v3/ScriptLegacyv3.hpp"

#include "v4/ObjectLegacyv4.hpp"
#include "v4/ScriptLegacyv4.hpp"

namespace Legacy
{

extern int32 OBJECT_BORDER_X1;
extern int32 OBJECT_BORDER_X2;
extern int32 OBJECT_BORDER_X3;
extern int32 OBJECT_BORDER_X4;
extern const int32 OBJECT_BORDER_Y1;
extern const int32 OBJECT_BORDER_Y2;
extern const int32 OBJECT_BORDER_Y3;
extern const int32 OBJECT_BORDER_Y4;

extern char scriptErrorMessage[0x400];

bool32 ConvertStringToInteger(const char *text, int32 *value);

} // namespace Legacy