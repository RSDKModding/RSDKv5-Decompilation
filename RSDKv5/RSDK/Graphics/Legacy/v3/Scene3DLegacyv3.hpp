
namespace Legacy
{

namespace v3
{

#define LEGACY_v3_VERTEXBUFFER_SIZE (0x1000)
#define LEGACY_v3_FACEBUFFER_SIZE   (0x400)

extern Face faceBuffer[LEGACY_v3_FACEBUFFER_SIZE];
extern Vertex vertexBuffer[LEGACY_v3_VERTEXBUFFER_SIZE];
extern Vertex vertexBufferT[LEGACY_v3_VERTEXBUFFER_SIZE];

extern DrawListEntry3D drawList3D[LEGACY_v3_FACEBUFFER_SIZE];

void SetIdentityMatrix(Matrix *matrix);
void MatrixMultiply(Matrix *matrixA, Matrix *matrixB);
void MatrixTranslateXYZ(Matrix *Matrix, int32 x, int32 y, int32 z);
void MatrixScaleXYZ(Matrix *matrix, int32 scaleX, int32 scaleY, int32 scaleZ);
void MatrixRotateX(Matrix *matrix, int32 rotationX);
void MatrixRotateY(Matrix *matrix, int32 rotationY);
void MatrixRotateZ(Matrix *matrix, int32 rotationZ);
void MatrixRotateXYZ(Matrix *matrix, int32 rotationX, int32 rotationY, int32 rotationZ);
void TransformVertexBuffer();
void TransformVerticies(Matrix *matrix, int32 startIndex, int32 endIndex);
void Sort3DDrawList();
void Draw3DScene(int32 spriteSheetID);

} // namespace v3

} // namespace Legacy