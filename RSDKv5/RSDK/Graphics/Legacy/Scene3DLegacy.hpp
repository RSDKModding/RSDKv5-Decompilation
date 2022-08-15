
namespace Legacy
{

enum FaceFlags {
    FACE_FLAG_TEXTURED_3D      = 0,
    FACE_FLAG_TEXTURED_2D      = 1,
    FACE_FLAG_COLORED_3D       = 2,
    FACE_FLAG_COLORED_2D       = 3,
    FACE_FLAG_FADED            = 4,
    FACE_FLAG_TEXTURED_C       = 5,
    FACE_FLAG_TEXTURED_C_BLEND = 6,
    FACE_FLAG_3DSPRITE         = 7
};

enum MatrixTypes {
    MAT_WORLD = 0,
    MAT_VIEW  = 1,
    MAT_TEMP  = 2,
};

struct Matrix {
    int32 values[4][4];
};

struct Vertex {
    int32 x;
    int32 y;
    int32 z;
    int32 u;
    int32 v;
};

struct Face {
    int32 a;
    int32 b;
    int32 c;
    int32 d;
    uint32 color;
    int32 flag;
};

struct DrawListEntry3D {
    int32 faceID;
    int32 depth;
};

extern int32 vertexCount;
extern int32 faceCount;

extern Matrix matFinal;
extern Matrix matWorld;
extern Matrix matView;
extern Matrix matTemp;

extern int32 projectionX;
extern int32 projectionY;

extern int32 faceLineStart[SCREEN_YSIZE];
extern int32 faceLineEnd[SCREEN_YSIZE];

extern int32 faceLineStartU[SCREEN_YSIZE];
extern int32 faceLineEndU[SCREEN_YSIZE];
extern int32 faceLineStartV[SCREEN_YSIZE];
extern int32 faceLineEndV[SCREEN_YSIZE];

void ProcessScanEdge(Vertex *vertA, Vertex *vertB);
void ProcessScanEdgeUV(Vertex *vertA, Vertex *vertB);

} // namespace Legacy

#include "v3/Scene3DLegacyv3.hpp"
#include "v4/Scene3DLegacyv4.hpp"