
RSDK::Legacy::Face RSDK::Legacy::v3::faceBuffer[LEGACY_v3_FACEBUFFER_SIZE];
RSDK::Legacy::Vertex RSDK::Legacy::v3::vertexBuffer[LEGACY_v3_VERTEXBUFFER_SIZE];
RSDK::Legacy::Vertex RSDK::Legacy::v3::vertexBufferT[LEGACY_v3_VERTEXBUFFER_SIZE];

RSDK::Legacy::DrawListEntry3D RSDK::Legacy::v3::drawList3D[LEGACY_v3_FACEBUFFER_SIZE];

void RSDK::Legacy::v3::SetIdentityMatrix(Matrix *matrix)
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
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v3::MatrixMultiply(Matrix *matrixA, Matrix *matrixB)
{
    int32 output[16];

    for (int32 i = 0; i < 0x10; ++i) {
        uint32 RowB = i & 3;
        uint32 RowA = (i & 0xC) / 4;
        output[i] = (matrixA->values[RowA][3] * matrixB->values[3][RowB] >> 8) + (matrixA->values[RowA][2] * matrixB->values[2][RowB] >> 8)
                    + (matrixA->values[RowA][1] * matrixB->values[1][RowB] >> 8) + (matrixA->values[RowA][0] * matrixB->values[0][RowB] >> 8);
    }

    for (int32 i = 0; i < 0x10; ++i) matrixA->values[i / 4][i % 4] = output[i];
}
void RSDK::Legacy::v3::MatrixTranslateXYZ(Matrix *matrix, int32 x, int32 y, int32 z)
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
    matrix->values[3][0] = x;
    matrix->values[3][1] = y;
    matrix->values[3][2] = z;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v3::MatrixScaleXYZ(Matrix *matrix, int32 scaleX, int32 scaleY, int32 scaleZ)
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
void RSDK::Legacy::v3::MatrixRotateX(Matrix *matrix, int32 rotationX)
{
    if (rotationX < 0)
        rotationX = 0x200 - rotationX;
    rotationX &= 0x1FF;
    int32 sine             = sin512LookupTable[rotationX] >> 1;
    int32 cosine           = cos512LookupTable[rotationX] >> 1;
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
void RSDK::Legacy::v3::MatrixRotateY(Matrix *matrix, int32 rotationY)
{
    if (rotationY < 0)
        rotationY = 0x200 - rotationY;
    rotationY &= 0x1FF;
    int32 sine             = sin512LookupTable[rotationY] >> 1;
    int32 cosine           = cos512LookupTable[rotationY] >> 1;
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
void RSDK::Legacy::v3::MatrixRotateZ(Matrix *matrix, int32 rotationZ)
{
    if (rotationZ < 0)
        rotationZ = 0x200 - rotationZ;
    rotationZ &= 0x1FF;
    int32 sine             = sin512LookupTable[rotationZ] >> 1;
    int32 cosine           = cos512LookupTable[rotationZ] >> 1;
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
void RSDK::Legacy::v3::MatrixRotateXYZ(Matrix *matrix, int32 rotationX, int32 rotationY, int32 rotationZ)
{
    if (rotationX < 0)
        rotationX = 0x200 - rotationX;
    rotationX &= 0x1FF;
    if (rotationY < 0)
        rotationY = 0x200 - rotationY;
    rotationY &= 0x1FF;
    if (rotationZ < 0)
        rotationZ = 0x200 - rotationZ;
    rotationZ &= 0x1FF;
    int32 sineX   = sin512LookupTable[rotationX] >> 1;
    int32 cosineX = cos512LookupTable[rotationX] >> 1;
    int32 sineY   = sin512LookupTable[rotationY] >> 1;
    int32 cosineY = cos512LookupTable[rotationY] >> 1;
    int32 sineZ   = sin512LookupTable[rotationZ] >> 1;
    int32 cosineZ = cos512LookupTable[rotationZ] >> 1;

    matrix->values[0][0] = (sineZ * (sineY * sineX >> 8) >> 8) + (cosineZ * cosineY >> 8);
    matrix->values[0][1] = (sineZ * cosineY >> 8) - (cosineZ * (sineY * sineX >> 8) >> 8);
    matrix->values[0][2] = sineY * cosineX >> 8;
    matrix->values[0][3] = 0;
    matrix->values[1][0] = sineZ * -cosineX >> 8;
    matrix->values[1][1] = cosineZ * cosineX >> 8;
    matrix->values[1][2] = sineX;
    matrix->values[1][3] = 0;
    matrix->values[2][0] = (sineZ * (cosineY * sineX >> 8) >> 8) - (cosineZ * sineY >> 8);
    matrix->values[2][1] = (sineZ * -sineY >> 8) - (cosineZ * (cosineY * sineX >> 8) >> 8);
    matrix->values[2][2] = cosineY * cosineX >> 8;
    matrix->values[2][3] = 0;
    matrix->values[3][0] = 0;
    matrix->values[3][1] = 0;
    matrix->values[3][2] = 0;
    matrix->values[3][3] = 0x100;
}
void RSDK::Legacy::v3::TransformVertexBuffer()
{
    for (int32 y = 0; y < 4; ++y) {
        for (int32 x = 0; x < 4; ++x) {
            matFinal.values[y][x] = matWorld.values[y][x];
        }
    }
    MatrixMultiply(&matFinal, &matView);

    if (vertexCount <= 0)
        return;

    int32 inVertexID  = 0;
    int32 outVertexID = 0;
    do {
        int32 vx       = vertexBuffer[inVertexID].x;
        int32 vy       = vertexBuffer[inVertexID].y;
        int32 vz       = vertexBuffer[inVertexID].z;
        Vertex *vert = &vertexBufferT[inVertexID++];

        vert->x = (vx * matFinal.values[0][0] >> 8) + (vy * matFinal.values[1][0] >> 8) + (vz * matFinal.values[2][0] >> 8) + matFinal.values[3][0];
        vert->y = (vx * matFinal.values[0][1] >> 8) + (vy * matFinal.values[1][1] >> 8) + (vz * matFinal.values[2][1] >> 8) + matFinal.values[3][1];
        vert->z = (vx * matFinal.values[0][2] >> 8) + (vy * matFinal.values[1][2] >> 8) + (vz * matFinal.values[2][2] >> 8) + matFinal.values[3][2];
    } while (++outVertexID != vertexCount);
}
void RSDK::Legacy::v3::TransformVerticies(Matrix *matrix, int32 startIndex, int32 endIndex)
{
    if (startIndex > endIndex)
        return;

    do {
        int32 vx       = vertexBuffer[startIndex].x;
        int32 vy       = vertexBuffer[startIndex].y;
        int32 vz       = vertexBuffer[startIndex].z;
        Vertex *vert = &vertexBuffer[startIndex];
        vert->x      = (vx * matrix->values[0][0] >> 8) + (vy * matrix->values[1][0] >> 8) + (vz * matrix->values[2][0] >> 8) + matrix->values[3][0];
        vert->y      = (vx * matrix->values[0][1] >> 8) + (vy * matrix->values[1][1] >> 8) + (vz * matrix->values[2][1] >> 8) + matrix->values[3][1];
        vert->z      = (vx * matrix->values[0][2] >> 8) + (vy * matrix->values[1][2] >> 8) + (vz * matrix->values[2][2] >> 8) + matrix->values[3][2];
    } while (++startIndex < endIndex);
}
void RSDK::Legacy::v3::Sort3DDrawList()
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
                int32 faceID               = drawList3D[j].faceID;
                int32 depth                = drawList3D[j].depth;
                drawList3D[j].faceID     = drawList3D[j - 1].faceID;
                drawList3D[j].depth      = drawList3D[j - 1].depth;
                drawList3D[j - 1].faceID = faceID;
                drawList3D[j - 1].depth  = depth;
            }
        }
    }
}
void RSDK::Legacy::v3::Draw3DScene(int32 spriteSheetID)
{
    Vertex quad[4];
    for (int32 i = 0; i < faceCount; ++i) {
        Face *face = &faceBuffer[drawList3D[i].faceID];
        memset(quad, 0, 4 * sizeof(Vertex));

        switch (face->flag) {
            default: break;

            case FACE_FLAG_TEXTURED_3D:
                if (vertexBufferT[face->a].z > 0x100 && vertexBufferT[face->b].z > 0x100 && vertexBufferT[face->c].z > 0x100
                    && vertexBufferT[face->d].z > 0x100) {
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
                quad[0].x = vertexBuffer[face->a].x;
                quad[0].y = vertexBuffer[face->a].y;
                quad[1].x = vertexBuffer[face->b].x;
                quad[1].y = vertexBuffer[face->b].y;
                quad[2].x = vertexBuffer[face->c].x;
                quad[2].y = vertexBuffer[face->c].y;
                quad[3].x = vertexBuffer[face->d].x;
                quad[3].y = vertexBuffer[face->d].y;
                quad[0].u = vertexBuffer[face->a].u;
                quad[0].v = vertexBuffer[face->a].v;
                quad[1].u = vertexBuffer[face->b].u;
                quad[1].v = vertexBuffer[face->b].v;
                quad[2].u = vertexBuffer[face->c].u;
                quad[2].v = vertexBuffer[face->c].v;
                quad[3].u = vertexBuffer[face->d].u;
                quad[3].v = vertexBuffer[face->d].v;
                DrawTexturedFace(quad, spriteSheetID);
                break;

            case FACE_FLAG_COLORED_3D:
                if (vertexBufferT[face->a].z > 0x100 && vertexBufferT[face->b].z > 0x100 && vertexBufferT[face->c].z > 0x100
                    && vertexBufferT[face->d].z > 0x100) {
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
                quad[0].x = vertexBuffer[face->a].x;
                quad[0].y = vertexBuffer[face->a].y;
                quad[1].x = vertexBuffer[face->b].x;
                quad[1].y = vertexBuffer[face->b].y;
                quad[2].x = vertexBuffer[face->c].x;
                quad[2].y = vertexBuffer[face->c].y;
                quad[3].x = vertexBuffer[face->d].x;
                quad[3].y = vertexBuffer[face->d].y;
                DrawFace(quad, face->color);
                break;
        }
    }
}
