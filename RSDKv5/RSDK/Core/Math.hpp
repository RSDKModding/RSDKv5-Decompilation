#ifndef MATH_H
#define MATH_H

namespace RSDK
{

// not "math" but works best here
#define INT_TO_VOID(x) (void *)(size_t)(x)
#define VOID_TO_INT(x) (int32)(size_t)(x)

#define MIN(a, b)                      ((a) < (b) ? (a) : (b))
#define MAX(a, b)                      ((a) > (b) ? (a) : (b))
#define CLAMP(value, minimum, maximum) (((value) < (minimum)) ? (minimum) : (((value) > (maximum)) ? (maximum) : (value)))

#define TO_FIXED(x)   ((x) << 16)
#define FROM_FIXED(x) ((x) >> 16)

// M_PI is *too* accurate, so use this instead
#define RSDK_PI (3.1415927f)

struct Vector2 {
    int32 x;
    int32 y;
};

#define MEM_ZERO(x) memset(&(x), 0, sizeof((x)))

extern int32 sin1024LookupTable[0x400];
extern int32 cos1024LookupTable[0x400];
extern int32 tan1024LookupTable[0x400];
extern int32 asin1024LookupTable[0x400];
extern int32 acos1024LookupTable[0x400];

extern int32 sin512LookupTable[0x200];
extern int32 cos512LookupTable[0x200];
extern int32 tan512LookupTable[0x200];
extern int32 asin512LookupTable[0x200];
extern int32 acos512LookupTable[0x200];

extern int32 sin256LookupTable[0x100];
extern int32 cos256LookupTable[0x100];
extern int32 tan256LookupTable[0x100];
extern int32 asin256LookupTable[0x100];
extern int32 acos256LookupTable[0x100];

extern uint8 arcTan256LookupTable[0x100 * 0x100];

// Setup angles
void ClearTrigLookupTables();
void CalculateTrigAngles();

inline int32 Sin1024(int32 angle) { return sin1024LookupTable[angle & 0x3FF]; }
inline int32 Cos1024(int32 angle) { return cos1024LookupTable[angle & 0x3FF]; }
inline int32 Tan1024(int32 angle) { return tan1024LookupTable[angle & 0x3FF]; }
inline int32 ASin1024(int32 angle)
{
    if (angle > 0x3FF)
        return 0;
    if (angle < 0)
        return -asin1024LookupTable[-angle];
    return asin1024LookupTable[angle];
}
inline int32 ACos1024(int32 angle)
{
    if (angle > 0x3FF)
        return 0;
    if (angle < 0)
        return -acos1024LookupTable[-angle];
    return acos1024LookupTable[angle];
}

inline int32 Sin512(int32 angle) { return sin512LookupTable[angle & 0x1FF]; }
inline int32 Cos512(int32 angle) { return cos512LookupTable[angle & 0x1FF]; }
inline int32 Tan512(int32 angle) { return tan512LookupTable[angle & 0x1FF]; }
inline int32 ASin512(int32 angle)
{
    if (angle > 0x1FF)
        return 0;
    if (angle < 0)
        return -asin512LookupTable[-angle];
    return asin512LookupTable[angle];
}
inline int32 ACos512(int32 angle)
{
    if (angle > 0x1FF)
        return 0;
    if (angle < 0)
        return -acos512LookupTable[-angle];
    return acos512LookupTable[angle];
}

inline int32 Sin256(int32 angle) { return sin256LookupTable[angle & 0xFF]; }
inline int32 Cos256(int32 angle) { return cos256LookupTable[angle & 0xFF]; }
inline int32 Tan256(int32 angle) { return tan256LookupTable[angle & 0xFF]; }
inline int32 ASin256(int32 angle)
{
    if (angle > 0xFF)
        return 0;
    if (angle < 0)
        return -asin256LookupTable[-angle];
    return asin256LookupTable[angle];
}
inline int32 ACos256(int32 angle)
{
    if (angle > 0xFF)
        return 0;
    if (angle < 0)
        return -acos256LookupTable[-angle];
    return acos256LookupTable[angle];
}

// Get Arc Tan value
uint8 ArcTanLookup(int32 x, int32 y);

extern uint32 randSeed;

inline void SetRandSeed(int32 key) { randSeed = key; }
inline int32 Rand(int32 min, int32 max)
{
    uint32 seed1 = randSeed * 0x41c64e6d + 0x3039;
    uint32 seed2 = seed1 * 0x41c64e6d + 0x3039;
    randSeed = seed2 * 0x41c64e6d + 0x3039;
    int32 res = ((seed1 >> 0x10 & 0x7ff) << 10 ^ seed2 >> 0x10 & 0x7ff) << 10 ^ randSeed >> 0x10 & 0x7ff;
    if (min < max) {
        return min + res % (max - min);
    }
    return max + res % (min - max);
}
inline int32 RandSeeded(int32 min, int32 max, int32 *randSeed)
{
    if (!randSeed)
        return 0;

    uint32 seed1 = *randSeed * 0x41c64e6d + 0x3039;
    uint32 seed2 = seed1 * 0x41c64e6d + 0x3039;
    *randSeed = seed2 * 0x41c64e6d + 0x3039;
    int32 res = ((seed1 >> 0x10 & 0x7ff) << 10 ^ seed2 >> 0x10 & 0x7ff) << 10 ^ *randSeed >> 0x10 & 0x7ff;
    if (min < max) {
        return min + res % (max - min);
    }
    return max + res % (min - max);
}

} // namespace RSDK

#endif // !MATH_H
