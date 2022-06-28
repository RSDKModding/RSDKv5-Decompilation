
namespace Legacy
{

namespace v4
{

#define LEGACY_v4_VERTEXBUFFER_SIZE (0x1000)
#define LEGACY_v4_FACEBUFFER_SIZE   (0x400)

extern Face faceBuffer[LEGACY_v4_FACEBUFFER_SIZE];
extern Vertex vertexBuffer[LEGACY_v4_VERTEXBUFFER_SIZE];
extern Vertex vertexBufferT[LEGACY_v4_VERTEXBUFFER_SIZE];

extern DrawListEntry3D drawList3D[LEGACY_v4_FACEBUFFER_SIZE];

extern int32 fogColor;
extern int32 fogStrength;

void SetIdentityMatrix(Matrix *matrix);
void MatrixMultiply(Matrix *matrixA, Matrix *matrixB);
void MatrixTranslateXYZ(Matrix *Matrix, int32 x, int32 y, int32 z);
void MatrixScaleXYZ(Matrix *matrix, int32 scaleX, int32 scaleY, int32 scaleZ);
void MatrixRotateX(Matrix *matrix, int32 rotationX);
void MatrixRotateY(Matrix *matrix, int32 rotationY);
void MatrixRotateZ(Matrix *matrix, int32 rotationZ);
void MatrixRotateXYZ(Matrix *matrix, int16 rotationX, int16 rotationY, int16 rotationZ);
void MatrixInverse(Matrix *matrix);
void TransformVertexBuffer();
void TransformVertices(Matrix *matrix, int32 startIndex, int32 endIndex);
void Sort3DDrawList();
void Draw3DScene(int32 spriteSheetID);

}

}