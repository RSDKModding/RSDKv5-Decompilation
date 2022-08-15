
RSDK::Legacy::Face RSDK::Legacy::v4::faceBuffer[LEGACY_v4_FACEBUFFER_SIZE];
RSDK::Legacy::Vertex RSDK::Legacy::v4::vertexBuffer[LEGACY_v4_VERTEXBUFFER_SIZE];
RSDK::Legacy::Vertex RSDK::Legacy::v4::vertexBufferT[LEGACY_v4_VERTEXBUFFER_SIZE];

RSDK::Legacy::DrawListEntry3D RSDK::Legacy::v4::drawList3D[LEGACY_v4_FACEBUFFER_SIZE];

int32 RSDK::Legacy::v4::fogColor    = 0;
int32 RSDK::Legacy::v4::fogStrength = 0;

void RSDK::Legacy::v4::SetIdentityMatrix(Matrix *matrix)
{
    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = 0x100;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixMultiply(Matrix *matrixA, Matrix *matrixB)
{
    int32 output[16];

    for (int32 i = 0; i < 0x10; ++i) {
        uint32 rowA = i / 4;
        uint32 rowB = i % 4;
        output[i]   = (matrixA->values[rowA][3] * matrixB->values[3][rowB] >> 8) + (matrixA->values[rowA][2] * matrixB->values[2][rowB] >> 8)
                    + (matrixA->values[rowA][1] * matrixB->values[1][rowB] >> 8) + (matrixA->values[rowA][0] * matrixB->values[0][rowB] >> 8);
    }

    for (int32 i = 0; i < 0x10; ++i) matrixA->values[i / 4][i % 4] = output[i];
}
void RSDK::Legacy::v4::MatrixTranslateXYZ(Matrix *matrix, int32 XPos, int32 YPos, int32 ZPos)
{
    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = 0x100;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = XPos;
    matrix->values[3][1] = YPos;
    matrix->values[3][2] = ZPos;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixScaleXYZ(Matrix *matrix, int32 scaleX, int32 scaleY, int32 scaleZ)
{
    matrix->values[0][0] = scaleX;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = scaleY;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = 0;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = scaleZ;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixRotateX(Matrix *matrix, int32 rotationX)
{
    int32 sine   = sin512LookupTable[rotationX & 0x1FF] >> 1;
    int32 cosine = cos512LookupTable[rotationX & 0x1FF] >> 1;

    matrix->values[0][0] = 0x100;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = 0;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = cosine;
    matrix->values[1][2] = sine;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = 0;
    matrix->values[2][1] = -sine;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixRotateY(Matrix *matrix, int32 rotationY)
{
    int32 sine   = sin512LookupTable[rotationY & 0x1FF] >> 1;
    int32 cosine = cos512LookupTable[rotationY & 0x1FF] >> 1;

    matrix->values[0][0] = cosine;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = sine;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = -sine;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixRotateZ(Matrix *matrix, int32 rotationZ)
{
    int32 sine           = sin512LookupTable[rotationZ & 0x1FF] >> 1;
    int32 cosine         = cos512LookupTable[rotationZ & 0x1FF] >> 1;
    matrix->values[0][0] = cosine;
    matrix->values[0][1] = 0;
    matrix->values[0][2] = sine;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = 0;
    matrix->values[1][1] = 0x100;
    matrix->values[1][2] = 0;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = -sine;
    matrix->values[2][1] = 0;
    matrix->values[2][2] = cosine;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixRotateXYZ(Matrix *matrix, int16 rotationX, int16 rotationY, int16 rotationZ)
{
    int32 sinX = sin512LookupTable[rotationX & 0x1FF] >> 1;
    int32 cosX = cos512LookupTable[rotationX & 0x1FF] >> 1;
    int32 sinY = sin512LookupTable[rotationY & 0x1FF] >> 1;
    int32 cosY = cos512LookupTable[rotationY & 0x1FF] >> 1;
    int32 sinZ = sin512LookupTable[rotationZ & 0x1FF] >> 1;
    int32 cosZ = cos512LookupTable[rotationZ & 0x1FF] >> 1;

    matrix->values[0][0] = (cosZ * cosY >> 8) + (sinZ * (sinY * sinX >> 8) >> 8);
    matrix->values[0][1] = (sinZ * cosY >> 8) - (cosZ * (sinY * sinX >> 8) >> 8);
    matrix->values[0][2] = sinY * cosX >> 8;
    matrix->values[0][3] = 0;

    matrix->values[1][0] = sinZ * -cosX >> 8;
    matrix->values[1][1] = cosZ * cosX >> 8;
    matrix->values[1][2] = sinX;
    matrix->values[1][3] = 0;

    matrix->values[2][0] = (sinZ * (cosY * sinX >> 8) >> 8) - (cosZ * sinY >> 8);
    matrix->values[2][1] = (sinZ * -sinY >> 8) - (cosZ * (cosY * sinX >> 8) >> 8);
    matrix->values[2][2] = cosY * cosX >> 8;
    matrix->values[2][3] = 0;

    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v4::MatrixInverse(Matrix *matrix)
{
    double inv[16], det;
    double m[16];
    for (int32 y = 0; y < 4; ++y) {
        for (int32 x = 0; x < 4; ++x) {
            m[(y << 2) + x] = matrix->values[y][x] / 256.0;
        }
    }

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return;

    det = 1.0 / det;

    for (int32 i = 0; i < 0x10; ++i) inv[i] = (int32)((inv[i] * det) * 256);
    for (int32 i = 0; i < 0x10; ++i) matrix->values[i / 4][i % 4] = (int32)inv[i];
}
void RSDK::Legacy::v4::TransformVertexBuffer()
{
    matFinal.values[0][0] = matWorld.values[0][0];
    matFinal.values[0][1] = matWorld.values[0][1];
    matFinal.values[0][2] = matWorld.values[0][2];
    matFinal.values[0][3] = matWorld.values[0][3];

    matFinal.values[1][0] = matWorld.values[1][0];
    matFinal.values[1][1] = matWorld.values[1][1];
    matFinal.values[1][2] = matWorld.values[1][2];
    matFinal.values[1][3] = matWorld.values[1][3];

    matFinal.values[2][0] = matWorld.values[2][0];
    matFinal.values[2][1] = matWorld.values[2][1];
    matFinal.values[2][2] = matWorld.values[2][2];
    matFinal.values[2][3] = matWorld.values[2][3];

    matFinal.values[3][0] = matWorld.values[3][0];
    matFinal.values[3][1] = matWorld.values[3][1];
    matFinal.values[3][2] = matWorld.values[3][2];
    matFinal.values[3][3] = matWorld.values[3][3];
    MatrixMultiply(&matFinal, &matView);

    for (int32 v = 0; v < vertexCount; ++v) {
        int32 vx = vertexBuffer[v].x;
        int32 vy = vertexBuffer[v].y;
        int32 vz = vertexBuffer[v].z;

        vertexBufferT[v].x =
            (vx * matFinal.values[0][0] >> 8) + (vy * matFinal.values[1][0] >> 8) + (vz * matFinal.values[2][0] >> 8) + matFinal.values[3][0];
        vertexBufferT[v].y =
            (vx * matFinal.values[0][1] >> 8) + (vy * matFinal.values[1][1] >> 8) + (vz * matFinal.values[2][1] >> 8) + matFinal.values[3][1];
        vertexBufferT[v].z =
            (vx * matFinal.values[0][2] >> 8) + (vy * matFinal.values[1][2] >> 8) + (vz * matFinal.values[2][2] >> 8) + matFinal.values[3][2];
    }
}
void RSDK::Legacy::v4::TransformVertices(Matrix *matrix, int32 startIndex, int32 endIndex)
{
    for (int32 v = startIndex; v < endIndex; ++v) {
        int32 vx     = vertexBuffer[v].x;
        int32 vy     = vertexBuffer[v].y;
        int32 vz     = vertexBuffer[v].z;
        Vertex *vert = &vertexBuffer[v];
        vert->x      = (vx * matrix->values[0][0] >> 8) + (vy * matrix->values[1][0] >> 8) + (vz * matrix->values[2][0] >> 8) + matrix->values[3][0];
        vert->y      = (vx * matrix->values[0][1] >> 8) + (vy * matrix->values[1][1] >> 8) + (vz * matrix->values[2][1] >> 8) + matrix->values[3][1];
        vert->z      = (vx * matrix->values[0][2] >> 8) + (vy * matrix->values[1][2] >> 8) + (vz * matrix->values[2][2] >> 8) + matrix->values[3][2];
    }
}
void RSDK::Legacy::v4::Sort3DDrawList()
{
    for (int32 i = 0; i < faceCount; ++i) {
        drawList3D[i].depth = (vertexBufferT[faceBuffer[i].d].z + vertexBufferT[faceBuffer[i].c].z + vertexBufferT[faceBuffer[i].b].z
                               + vertexBufferT[faceBuffer[i].a].z)
                              >> 2;
        drawList3D[i].faceID = i;
    }

    for (int32 i = 0; i < faceCount; ++i) {
        for (int32 j = faceCount - 1; j > i; --j) {
            if (drawList3D[j].depth > drawList3D[j - 1].depth) {
                int32 faceID             = drawList3D[j].faceID;
                int32 depth              = drawList3D[j].depth;
                drawList3D[j].faceID     = drawList3D[j - 1].faceID;
                drawList3D[j].depth      = drawList3D[j - 1].depth;
                drawList3D[j - 1].faceID = faceID;
                drawList3D[j - 1].depth  = depth;
            }
        }
    }
}
void RSDK::Legacy::v4::Draw3DScene(int32 spriteSheetID)
{
    Vertex quad[4];
    for (int32 i = 0; i < faceCount; ++i) {
        Face *face = &faceBuffer[drawList3D[i].faceID];
        memset(quad, 0, 4 * sizeof(Vertex));
        switch (face->flag) {
            default: break;
            case FACE_FLAG_TEXTURED_3D:
                if (vertexBufferT[face->a].z > 0 && vertexBufferT[face->b].z > 0 && vertexBufferT[face->c].z > 0 && vertexBufferT[face->d].z > 0) {
                    quad[0].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->a].x / vertexBufferT[face->a].z;
                    quad[0].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->a].y / vertexBufferT[face->a].z;
                    quad[1].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->b].x / vertexBufferT[face->b].z;
                    quad[1].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->b].y / vertexBufferT[face->b].z;
                    quad[2].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->c].x / vertexBufferT[face->c].z;
                    quad[2].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->c].y / vertexBufferT[face->c].z;
                    quad[3].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->d].x / vertexBufferT[face->d].z;
                    quad[3].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->d].y / vertexBufferT[face->d].z;
                    quad[0].u = vertexBuffer[face->a].u;
                    quad[0].v = vertexBuffer[face->a].v;
                    quad[1].u = vertexBuffer[face->b].u;
                    quad[1].v = vertexBuffer[face->b].v;
                    quad[2].u = vertexBuffer[face->c].u;
                    quad[2].v = vertexBuffer[face->c].v;
                    quad[3].u = vertexBuffer[face->d].u;
                    quad[3].v = vertexBuffer[face->d].v;
                    DrawTexturedFace(quad, spriteSheetID);
                }
                break;
            case FACE_FLAG_TEXTURED_2D:
                if (vertexBufferT[face->a].z >= 0 && vertexBufferT[face->b].z >= 0 && vertexBufferT[face->c].z >= 0
                    && vertexBufferT[face->d].z >= 0) {
                    quad[0].x = vertexBufferT[face->a].x;
                    quad[0].y = vertexBufferT[face->a].y;
                    quad[1].x = vertexBufferT[face->b].x;
                    quad[1].y = vertexBufferT[face->b].y;
                    quad[2].x = vertexBufferT[face->c].x;
                    quad[2].y = vertexBufferT[face->c].y;
                    quad[3].x = vertexBufferT[face->d].x;
                    quad[3].y = vertexBufferT[face->d].y;
                    quad[0].u = vertexBuffer[face->a].u;
                    quad[0].v = vertexBuffer[face->a].v;
                    quad[1].u = vertexBuffer[face->b].u;
                    quad[1].v = vertexBuffer[face->b].v;
                    quad[2].u = vertexBuffer[face->c].u;
                    quad[2].v = vertexBuffer[face->c].v;
                    quad[3].u = vertexBuffer[face->d].u;
                    quad[3].v = vertexBuffer[face->d].v;
                    DrawTexturedFace(quad, spriteSheetID);
                }
                break;
            case FACE_FLAG_COLORED_3D:
                if (vertexBufferT[face->a].z > 0 && vertexBufferT[face->b].z > 0 && vertexBufferT[face->c].z > 0 && vertexBufferT[face->d].z > 0) {
                    quad[0].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->a].x / vertexBufferT[face->a].z;
                    quad[0].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->a].y / vertexBufferT[face->a].z;
                    quad[1].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->b].x / vertexBufferT[face->b].z;
                    quad[1].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->b].y / vertexBufferT[face->b].z;
                    quad[2].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->c].x / vertexBufferT[face->c].z;
                    quad[2].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->c].y / vertexBufferT[face->c].z;
                    quad[3].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->d].x / vertexBufferT[face->d].z;
                    quad[3].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->d].y / vertexBufferT[face->d].z;
                    DrawFace(quad, face->color);
                }
                break;
            case FACE_FLAG_COLORED_2D:
                if (vertexBufferT[face->a].z >= 0 && vertexBufferT[face->b].z >= 0 && vertexBufferT[face->c].z >= 0
                    && vertexBufferT[face->d].z >= 0) {
                    quad[0].x = vertexBufferT[face->a].x;
                    quad[0].y = vertexBufferT[face->a].y;
                    quad[1].x = vertexBufferT[face->b].x;
                    quad[1].y = vertexBufferT[face->b].y;
                    quad[2].x = vertexBufferT[face->c].x;
                    quad[2].y = vertexBufferT[face->c].y;
                    quad[3].x = vertexBufferT[face->d].x;
                    quad[3].y = vertexBufferT[face->d].y;
                    DrawFace(quad, face->color);
                }
                break;
            case FACE_FLAG_FADED:
                if (vertexBufferT[face->a].z > 0 && vertexBufferT[face->b].z > 0 && vertexBufferT[face->c].z > 0 && vertexBufferT[face->d].z > 0) {
                    quad[0].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->a].x / vertexBufferT[face->a].z;
                    quad[0].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->a].y / vertexBufferT[face->a].z;
                    quad[1].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->b].x / vertexBufferT[face->b].z;
                    quad[1].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->b].y / vertexBufferT[face->b].z;
                    quad[2].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->c].x / vertexBufferT[face->c].z;
                    quad[2].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->c].y / vertexBufferT[face->c].z;
                    quad[3].x = SCREEN_CENTERX + projectionX * vertexBufferT[face->d].x / vertexBufferT[face->d].z;
                    quad[3].y = SCREEN_CENTERY - projectionY * vertexBufferT[face->d].y / vertexBufferT[face->d].z;

                    int32 fogStr = 0;
                    if ((drawList3D[i].depth - 0x8000) >> 8 >= 0)
                        fogStr = (drawList3D[i].depth - 0x8000) >> 8;
                    if (fogStr > fogStrength)
                        fogStr = fogStrength;

                    DrawFadedFace(quad, face->color, fogColor, 0xFF - fogStr);
                }
                break;
            case FACE_FLAG_TEXTURED_C:
                if (vertexBufferT[face->a].z > 0) {
                    // [face->a].uv == sprite center
                    // [face->b].uv == ???
                    // [face->c].uv == sprite extend (how far to each edge X & Y)
                    // [face->d].uv == unused

                    quad[0].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x - vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[0].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y + vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[1].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x + vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[1].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y + vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[2].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x - vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[2].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y - vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[3].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x + vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[3].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y - vertexBuffer[face->b].v) / vertexBufferT[face->a].z;

                    quad[0].u = vertexBuffer[face->a].u - vertexBuffer[face->c].u;
                    quad[0].v = vertexBuffer[face->a].v - vertexBuffer[face->c].v;
                    quad[1].u = vertexBuffer[face->a].u + vertexBuffer[face->c].u;
                    quad[1].v = vertexBuffer[face->a].v - vertexBuffer[face->c].v;
                    quad[2].u = vertexBuffer[face->a].u - vertexBuffer[face->c].u;
                    quad[2].v = vertexBuffer[face->a].v + vertexBuffer[face->c].v;
                    quad[3].u = vertexBuffer[face->a].u + vertexBuffer[face->c].u;
                    quad[3].v = vertexBuffer[face->a].v + vertexBuffer[face->c].v;

                    DrawTexturedFace(quad, face->color);
                }
                break;
            case FACE_FLAG_TEXTURED_C_BLEND:
                if (vertexBufferT[face->a].z > 0) {
                    // See above, its the same just blended

                    quad[0].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x - vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[0].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y + vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[1].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x + vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[1].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y + vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[2].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x - vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[2].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y - vertexBuffer[face->b].v) / vertexBufferT[face->a].z;
                    quad[3].x = SCREEN_CENTERX + projectionX * (vertexBufferT[face->a].x + vertexBuffer[face->b].u) / vertexBufferT[face->a].z;
                    quad[3].y = SCREEN_CENTERY - projectionY * (vertexBufferT[face->a].y - vertexBuffer[face->b].v) / vertexBufferT[face->a].z;

                    quad[0].u = vertexBuffer[face->a].u - vertexBuffer[face->c].u;
                    quad[0].v = vertexBuffer[face->a].v - vertexBuffer[face->c].v;
                    quad[1].u = vertexBuffer[face->a].u + vertexBuffer[face->c].u;
                    quad[1].v = vertexBuffer[face->a].v - vertexBuffer[face->c].v;
                    quad[2].u = vertexBuffer[face->a].u - vertexBuffer[face->c].u;
                    quad[2].v = vertexBuffer[face->a].v + vertexBuffer[face->c].v;
                    quad[3].u = vertexBuffer[face->a].u + vertexBuffer[face->c].u;
                    quad[3].v = vertexBuffer[face->a].v + vertexBuffer[face->c].v;

                    DrawTexturedFaceBlended(quad, face->color);
                }
                break;
            case FACE_FLAG_3DSPRITE:
                if (vertexBufferT[face->a].z > 0) {
                    int32 xpos = SCREEN_CENTERX + projectionX * vertexBufferT[face->a].x / vertexBufferT[face->a].z;
                    int32 ypos = SCREEN_CENTERY - projectionY * vertexBufferT[face->a].y / vertexBufferT[face->a].z;

                    ObjectScript *scriptInfo = &objectScriptList[vertexBuffer[face->a].u];
                    SpriteFrame *frame       = &scriptFrames[scriptInfo->frameListOffset + vertexBuffer[face->b].u];

                    switch (vertexBuffer[face->a].v) {
                        case FX_SCALE:
                            DrawSpriteScaled(vertexBuffer[face->b].v, xpos, ypos, -frame->pivotX, -frame->pivotY, vertexBuffer[face->c].u,
                                             vertexBuffer[face->c].u, frame->width, frame->height, frame->sprX, frame->sprY,
                                             scriptInfo->spriteSheetID);
                            break;
                        case FX_ROTATE:
                            DrawSpriteRotated(vertexBuffer[face->b].v, xpos, ypos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY,
                                              frame->width, frame->height, vertexBuffer[face->c].v, scriptInfo->spriteSheetID);
                            break;
                        case FX_ROTOZOOM:
                            DrawSpriteRotozoom(vertexBuffer[face->b].v, xpos, ypos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY,
                                               frame->width, frame->height, vertexBuffer[face->c].v, vertexBuffer[face->c].u,
                                               scriptInfo->spriteSheetID);
                            break;
                    }
                }
                break;
        }
    }
}
