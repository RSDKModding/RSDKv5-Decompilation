
namespace Legacy
{

namespace v3
{

extern int32 yScrollA;
extern int32 yScrollB;
extern int32 xScrollA;
extern int32 xScrollB;
extern int32 yScrollMove;

void InitFirstStage();
void ProcessStage();
void HandleCameras();

void LoadStageFiles();
void LoadActLayout();
void LoadStageBackground();

void SetPlayerScreenPosition(Player *player);
void SetPlayerScreenPositionCDStyle(Player *player);
void SetPlayerHLockedScreenPosition(Player *player);
void SetPlayerLockedScreenPosition(Player *player);

}

}