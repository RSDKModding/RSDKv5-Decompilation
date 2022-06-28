#include "v3/Scene3DLegacyv3.cpp"
#include "v4/Scene3DLegacyv4.cpp"

int32 RSDK::Legacy::vertexCount = 0;
int32 RSDK::Legacy::faceCount   = 0;

RSDK::Legacy::Matrix RSDK::Legacy::matFinal;
RSDK::Legacy::Matrix RSDK::Legacy::matWorld;
RSDK::Legacy::Matrix RSDK::Legacy::matView;
RSDK::Legacy::Matrix RSDK::Legacy::matTemp;

int32 RSDK::Legacy::projectionX = 136;
int32 RSDK::Legacy::projectionY = 160;

int32 RSDK::Legacy::faceLineStart[SCREEN_YSIZE];
int32 RSDK::Legacy::faceLineEnd[SCREEN_YSIZE];
int32 RSDK::Legacy::faceLineStartU[SCREEN_YSIZE];
int32 RSDK::Legacy::faceLineEndU[SCREEN_YSIZE];
int32 RSDK::Legacy::faceLineStartV[SCREEN_YSIZE];
int32 RSDK::Legacy::faceLineEndV[SCREEN_YSIZE];

void RSDK::Legacy::ProcessScanEdge(Vertex *vertA, Vertex *vertB)
{
    int32 bottom, top;

    if (vertA->y == vertB->y)
        return;
    if (vertA->y >= vertB->y) {
        top    = vertB->y;
        bottom = vertA->y + 1;
    }
    else {
        top    = vertA->y;
        bottom = vertB->y + 1;
    }
    if (top > SCREEN_YSIZE - 1 || bottom < 0)
        return;
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;
    int32 fullX  = vertA->x << 16;
    int32 deltaX = ((vertB->x - vertA->x) << 16) / (vertB->y - vertA->y);
    if (top < 0) {
        fullX -= top * deltaX;
        top = 0;
    }
    for (int32 i = top; i < bottom; ++i) {
        int32 trueX = fullX >> 16;
        if (trueX < faceLineStart[i])
            faceLineStart[i] = trueX;
        if (trueX > faceLineEnd[i])
            faceLineEnd[i] = trueX;
        fullX += deltaX;
    }
}
void RSDK::Legacy::ProcessScanEdgeUV(Vertex *vertA, Vertex *vertB)
{
    int32 bottom, top;

    if (vertA->y == vertB->y)
        return;
    if (vertA->y >= vertB->y) {
        top    = vertB->y;
        bottom = vertA->y + 1;
    }
    else {
        top    = vertA->y;
        bottom = vertB->y + 1;
    }
    if (top > SCREEN_YSIZE - 1 || bottom < 0)
        return;
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;

    int32 fullX  = vertA->x << 16;
    int32 fullU  = vertA->u << 16;
    int32 fullV  = vertA->v << 16;
    int32 deltaX = ((vertB->x - vertA->x) << 16) / (vertB->y - vertA->y);

    int32 deltaU = 0;
    if (vertA->u != vertB->u)
        deltaU = ((vertB->u - vertA->u) << 16) / (vertB->y - vertA->y);

    int32 deltaV = 0;
    if (vertA->v != vertB->v) {
        deltaV = ((vertB->v - vertA->v) << 16) / (vertB->y - vertA->y);
    }

    if (top < 0) {
        fullX -= top * deltaX;
        fullU -= top * deltaU;
        fullV -= top * deltaV;
        top = 0;
    }
    for (int32 i = top; i < bottom; ++i) {
        int32 trueX = fullX >> 16;
        if (trueX < faceLineStart[i]) {
            faceLineStart[i]  = trueX;
            faceLineStartU[i] = fullU;
            faceLineStartV[i] = fullV;
        }
        if (trueX > faceLineEnd[i]) {
            faceLineEnd[i]  = trueX;
            faceLineEndU[i] = fullU;
            faceLineEndV[i] = fullV;
        }
        fullX += deltaX;
        fullU += deltaU;
        fullV += deltaV;
    }
}