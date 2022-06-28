
namespace Legacy
{

namespace v4
{

enum StageModes {
    STAGEMODE_FROZEN = 3,
    STAGEMODE_2P,
};

void InitFirstStage();
void ProcessStage();
void HandleCameras();

void ProcessParallaxAutoScroll();

void LoadStageFiles();

void LoadActLayout();
void LoadStageBackground();

void SetPlayerScreenPosition(Entity *target);
void SetPlayerScreenPositionCDStyle(Entity *target);
void SetPlayerHLockedScreenPosition(Entity *target);
void SetPlayerLockedScreenPosition(Entity *target);
void SetPlayerScreenPositionFixed(Entity *target);
void SetPlayerScreenPositionStatic(Entity *target);
} // namespace v4

} // namespace Legacy