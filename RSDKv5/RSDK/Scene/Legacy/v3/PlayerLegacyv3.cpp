
RSDK::Legacy::v3::Player RSDK::Legacy::v3::playerList[LEGACY_v3_PLAYER_COUNT];
int32 RSDK::Legacy::v3::playerListPos     = 0;
int32 RSDK::Legacy::v3::activePlayer      = 0;
int32 RSDK::Legacy::v3::activePlayerCount = 1;

uint16 RSDK::Legacy::v3::upBuffer        = 0;
uint16 RSDK::Legacy::v3::downBuffer      = 0;
uint16 RSDK::Legacy::v3::leftBuffer      = 0;
uint16 RSDK::Legacy::v3::rightBuffer     = 0;
uint16 RSDK::Legacy::v3::jumpPressBuffer = 0;
uint16 RSDK::Legacy::v3::jumpHoldBuffer  = 0;

uint8 RSDK::Legacy::v3::nextLeaderPosID = 1;
uint8 RSDK::Legacy::v3::lastLeaderPosID = 0;

int16 RSDK::Legacy::v3::leaderPositionBufferX[16];
int16 RSDK::Legacy::v3::leaderPositionBufferY[16];

void RSDK::Legacy::v3::ProcessPlayerControl(Player *player)
{
    switch (player->controlMode) {
        default:
        case CONTROLMODE_NORMAL:
            player->up    = stickL[CONT_P1].keyUp.down || controller[CONT_P1].keyUp.down;
            player->down  = stickL[CONT_P1].keyDown.down || controller[CONT_P1].keyDown.down;
            player->left  = stickL[CONT_P1].keyLeft.down || controller[CONT_P1].keyLeft.down;
            player->right = stickL[CONT_P1].keyRight.down || controller[CONT_P1].keyRight.down;

            if (player->left && player->right) {
                player->left  = false;
                player->right = false;
            }

            player->jumpHold  = controller[CONT_P1].keyA.down || controller[CONT_P1].keyB.down || controller[CONT_P1].keyC.down;
            player->jumpPress = controller[CONT_P1].keyA.press || controller[CONT_P1].keyB.press || controller[CONT_P1].keyC.press;

            upBuffer <<= 1;
            upBuffer |= (uint8)player->up;

            downBuffer <<= 1;
            downBuffer |= (uint8)player->down;

            leftBuffer <<= 1;
            leftBuffer |= (uint8)player->left;

            rightBuffer <<= 1;
            rightBuffer |= (uint8)player->right;

            jumpPressBuffer <<= 1;
            jumpPressBuffer |= (uint8)player->jumpPress;

            jumpHoldBuffer <<= 1;
            jumpHoldBuffer |= (uint8)player->jumpHold;

            if (activePlayerCount >= 2 && !player->followPlayer1) {
                upBuffer <<= 1;
                upBuffer |= (uint8)player->up;

                downBuffer <<= 1;
                downBuffer |= (uint8)player->down;

                leftBuffer <<= 1;
                leftBuffer |= (uint8)player->left;

                rightBuffer <<= 1;
                rightBuffer |= (uint8)player->right;

                jumpPressBuffer <<= 1;
                jumpPressBuffer |= (uint8)player->jumpPress;

                jumpHoldBuffer <<= 1;
                jumpHoldBuffer |= (uint8)player->jumpHold;

                leaderPositionBufferX[nextLeaderPosID] = player->XPos >> 16;
                leaderPositionBufferY[nextLeaderPosID] = player->YPos >> 16;
                nextLeaderPosID                        = (nextLeaderPosID + 1) & 0xF;
                lastLeaderPosID                        = (nextLeaderPosID + 1) & 0xF;
            }
            break;

        case CONTROLMODE_NONE:
            if (activePlayerCount >= 2 && !player->followPlayer1) {
                upBuffer <<= 1;
                upBuffer |= (uint8)player->up;

                downBuffer <<= 1;
                downBuffer |= (uint8)player->down;

                leftBuffer <<= 1;
                leftBuffer |= (uint8)player->left;

                rightBuffer <<= 1;
                rightBuffer |= (uint8)player->right;

                jumpPressBuffer <<= 1;
                jumpPressBuffer |= (uint8)player->jumpPress;

                jumpHoldBuffer <<= 1;
                jumpHoldBuffer |= (uint8)player->jumpHold;

                leaderPositionBufferX[nextLeaderPosID] = player->XPos >> 16;
                leaderPositionBufferY[nextLeaderPosID] = player->YPos >> 16;
                nextLeaderPosID                        = (nextLeaderPosID + 1) & 0xF;
                lastLeaderPosID                        = (nextLeaderPosID + 1) & 0xF;
            }
            break;

        case CONTROLMODE_SIDEKICK:
            player->up        = upBuffer >> 15;
            player->down      = downBuffer >> 15;
            player->left      = leftBuffer >> 15;
            player->right     = rightBuffer >> 15;
            player->jumpPress = jumpPressBuffer >> 15;
            player->jumpHold  = jumpHoldBuffer >> 15;

            player->boundEntity->XPos = leaderPositionBufferX[lastLeaderPosID] << 16;
            player->boundEntity->YPos = leaderPositionBufferY[lastLeaderPosID] << 16;
            break;
    }
}