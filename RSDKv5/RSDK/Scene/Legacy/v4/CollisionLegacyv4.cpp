
int32 RSDK::Legacy::v4::collisionLeft   = 0;
int32 RSDK::Legacy::v4::collisionTop    = 0;
int32 RSDK::Legacy::v4::collisionRight  = 0;
int32 RSDK::Legacy::v4::collisionBottom = 0;

int32 RSDK::Legacy::v4::collisionTolerance = 0;

RSDK::Legacy::v4::CollisionSensor RSDK::Legacy::v4::sensors[7];

#if !RETRO_USE_ORIGINAL_CODE
int32 RSDK::Legacy::v4::AddDebugHitbox(uint8 type, Entity *entity, int32 left, int32 top, int32 right, int32 bottom)
{
    int32 i = 0;
    for (; i < debugHitboxCount; ++i) {
        if (debugHitboxList[i].hitbox.left == left && debugHitboxList[i].hitbox.top == top && debugHitboxList[i].hitbox.right == right
            && debugHitboxList[i].hitbox.bottom == bottom && debugHitboxList[i].pos.x == entity->xpos && debugHitboxList[i].pos.y == entity->ypos
            && debugHitboxList[i].entity == entity) {
            return i;
        }
    }

    if (i < DEBUG_HITBOX_COUNT) {
        debugHitboxList[i].type          = type;
        debugHitboxList[i].entity        = entity;
        debugHitboxList[i].collision     = 0;
        debugHitboxList[i].hitbox.left   = left;
        debugHitboxList[i].hitbox.top    = top;
        debugHitboxList[i].hitbox.right  = right;
        debugHitboxList[i].hitbox.bottom = bottom;
        debugHitboxList[i].pos.x         = entity ? entity->xpos : 0;
        debugHitboxList[i].pos.y         = entity ? entity->ypos : 0;

        int32 id = debugHitboxCount;
        debugHitboxCount++;
        return id;
    }

    return -1;
}
#endif

RSDK::Legacy::Hitbox *RSDK::Legacy::v4::GetHitbox(Entity *entity)
{
    AnimationFile *thisAnim = objectScriptList[entity->type].animFile;

    return &hitboxList[thisAnim->hitboxListOffset
                       + animFrames[animationList[thisAnim->aniListOffset + entity->animation].frameListOffset + entity->frame].hitboxID];
}

void RSDK::Legacy::v4::FindFloorPosition(Entity *player, CollisionSensor *sensor, int32 startY)
{
    int32 c     = 0;
    int32 angle = sensor->angle;
    int32 tsm1  = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = sensor->xpos >> 16;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = (sensor->ypos >> 16) - TILE_SIZE + i;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile += tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_LRB
                    && tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].floorMasks[c] >= 0x40)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF;
                            break;
                        }
                        case FLIP_X: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].floorMasks[c] >= 0x40)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF);
                            break;
                        }
                        case FLIP_Y: {
                            c = (XPos & 15) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].roofMasks[c] <= -0x40)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24));
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].roofMasks[c] <= -0x40)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle = 0x100 - (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24));
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if ((abs(sensor->angle - angle) > 0x20) && (abs(sensor->angle - 0x100 - angle) > 0x20)
                        && (abs(sensor->angle + 0x100 - angle) > 0x20)) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                        sensor->angle    = angle;
                        i                = TILE_SIZE * 3;
                    }
                    else if (sensor->ypos - startY > collisionTolerance || sensor->ypos - startY < -collisionTolerance) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::FindLWallPosition(Entity *player, CollisionSensor *sensor, int32 startX)
{
    int32 c     = 0;
    int32 angle = sensor->angle;
    int32 tsm1  = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = (sensor->xpos >> 16) - TILE_SIZE + i;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = sensor->ypos >> 16;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile      = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile            = tile + tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].lWallMasks[c] >= 0x40)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF00) >> 8);
                            break;
                        }
                        case FLIP_X: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].rWallMasks[c] <= -0x40)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF0000) >> 16);
                            break;
                        }
                        case FLIP_Y: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].lWallMasks[c] >= 0x40)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF00) >> 8));
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].rWallMasks[c] <= -0x40)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF0000) >> 16));
                            break;
                        }
                    }
                }
                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if (abs(angle - sensor->angle) > 0x20) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                        sensor->angle    = angle;
                        i                = TILE_SIZE * 3;
                    }
                    else if (sensor->xpos - startX > collisionTolerance || sensor->xpos - startX < -collisionTolerance) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::FindRoofPosition(Entity *player, CollisionSensor *sensor, int32 startY)
{
    int32 c     = 0;
    int32 angle = sensor->angle;
    int32 tsm1  = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = sensor->xpos >> 16;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = (sensor->ypos >> 16) + TILE_SIZE - i;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile      = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile            = tile + tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].roofMasks[c] <= -0x40)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24;
                            break;
                        }
                        case FLIP_X: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].roofMasks[c] <= -0x40)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24);
                            break;
                        }
                        case FLIP_Y: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].floorMasks[c] >= 0x40)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = (uint8)(0x180 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF));
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].floorMasks[c] >= 0x40)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (uint8)(0x180 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF));
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if (abs(sensor->angle - angle) <= 0x20) {
                        if (sensor->ypos - startY > collisionTolerance || sensor->ypos - startY < -collisionTolerance) {
                            sensor->ypos     = startY << 16;
                            sensor->collided = false;
                        }
                    }
                    else {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                        sensor->angle    = angle;
                        i                = TILE_SIZE * 3;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::FindRWallPosition(Entity *player, CollisionSensor *sensor, int32 startX)
{
    int32 c;
    int32 angle = sensor->angle;
    int32 tsm1  = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = (sensor->xpos >> 16) + TILE_SIZE - i;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = sensor->ypos >> 16;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile      = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile            = tile + tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].rWallMasks[c] <= -0x40)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = (uint8)((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF0000) >> 16);
                            break;
                        }
                        case FLIP_X: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].lWallMasks[c] >= 0x40)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF00) >> 8);
                            break;
                        }
                        case FLIP_Y: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].rWallMasks[c] <= -0x40)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF0000) >> 16));
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if (collisionMasks[player->collisionPlane].lWallMasks[c] >= 0x40)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF00) >> 8));
                            break;
                        }
                    }
                }
                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if (abs(sensor->angle - angle) > 0x20) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                        sensor->angle    = angle;
                        i                = TILE_SIZE * 3;
                    }
                    else if (sensor->xpos - startX > collisionTolerance || sensor->xpos - startX < -collisionTolerance) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}

void RSDK::Legacy::v4::FloorCollision(Entity *player, CollisionSensor *sensor)
{
    int32 c;
    int32 startY = sensor->ypos >> 16;
    int32 tsm1   = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = sensor->xpos >> 16;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = (sensor->ypos >> 16) - TILE_SIZE + i;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile += tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_LRB
                    && tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) <= collisionMasks[player->collisionPlane].floorMasks[c] - TILE_SIZE + i
                                || collisionMasks[player->collisionPlane].floorMasks[c] >= tsm1)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF;
                            break;
                        }
                        case FLIP_X: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) <= collisionMasks[player->collisionPlane].floorMasks[c] - TILE_SIZE + i
                                || collisionMasks[player->collisionPlane].floorMasks[c] >= tsm1)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF);
                            break;
                        }
                        case FLIP_Y: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) <= tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            uint8 cAngle     = (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24;
                            sensor->angle    = (uint8)(0x180 - cAngle);
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) <= tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle = 0x100 - (uint8)(0x180 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24));
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if (sensor->ypos - startY > (TILE_SIZE - 2)) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                    }
                    else if (sensor->ypos - startY < -(TILE_SIZE + 1)) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::LWallCollision(Entity *player, CollisionSensor *sensor)
{
    int32 c;
    int32 startX = sensor->xpos >> 16;
    int32 tsm1   = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = (sensor->xpos >> 16) - TILE_SIZE + i;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = sensor->ypos >> 16;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile += tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_TOP
                    && tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) <= collisionMasks[player->collisionPlane].lWallMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_X: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) <= tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_Y: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) <= collisionMasks[player->collisionPlane].lWallMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) <= tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] - TILE_SIZE + i)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->xpos - startX > tsm1) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                    else if (sensor->xpos - startX < -tsm1) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::RoofCollision(Entity *player, CollisionSensor *sensor)
{
    int32 c;
    int32 startY = sensor->ypos >> 16;
    int32 tsm1   = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = sensor->xpos >> 16;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = (sensor->ypos >> 16) + TILE_SIZE - i;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile += tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_TOP
                    && tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) >= collisionMasks[player->collisionPlane].roofMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24);
                            break;
                        }
                        case FLIP_X: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) >= collisionMasks[player->collisionPlane].roofMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->ypos     = collisionMasks[player->collisionPlane].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - ((collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF000000) >> 24);
                            break;
                        }
                        case FLIP_Y: {
                            c = (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) >= tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x180 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF);
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (XPos & tsm1) + (tileIndex << 4);
                            if ((YPos & tsm1) >= tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->ypos     = tsm1 - collisionMasks[player->collisionPlane].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                            sensor->collided = true;
                            sensor->angle    = 0x100 - (uint8)(0x180 - (collisionMasks[player->collisionPlane].angles[tileIndex] & 0xFF));
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->angle < 0)
                        sensor->angle += 0x100;

                    if (sensor->angle >= 0x100)
                        sensor->angle -= 0x100;

                    if (sensor->ypos - startY > (tsm1 - 1)) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                    }
                    else if (sensor->ypos - startY < -(tsm1 - 1)) {
                        sensor->ypos     = startY << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}
void RSDK::Legacy::v4::RWallCollision(Entity *player, CollisionSensor *sensor)
{
    int32 c;
    int32 startX = sensor->xpos >> 16;
    int32 tsm1   = (TILE_SIZE - 1);
    for (int32 i = 0; i < TILE_SIZE * 3; i += TILE_SIZE) {
        if (!sensor->collided) {
            int32 XPos   = (sensor->xpos >> 16) + TILE_SIZE - i;
            int32 chunkX = XPos >> 7;
            int32 tileX  = (XPos & 0x7F) >> 4;
            int32 YPos   = sensor->ypos >> 16;
            int32 chunkY = YPos >> 7;
            int32 tileY  = (YPos & 0x7F) >> 4;
            if (XPos > -1 && YPos > -1) {
                int32 tile = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
                tile += tileX + (tileY << 3);
                int32 tileIndex = tiles128x128.tileIndex[tile];
                if (tiles128x128.collisionFlags[player->collisionPlane][tile] != SOLID_TOP
                    && tiles128x128.collisionFlags[player->collisionPlane][tile] < SOLID_NONE) {
                    switch (tiles128x128.direction[tile]) {
                        case FLIP_NONE: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) >= collisionMasks[player->collisionPlane].rWallMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_X: {
                            c = (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) >= tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_Y: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) >= collisionMasks[player->collisionPlane].rWallMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->xpos     = collisionMasks[player->collisionPlane].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                        case FLIP_XY: {
                            c = tsm1 - (YPos & tsm1) + (tileIndex << 4);
                            if ((XPos & tsm1) >= tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + TILE_SIZE - i)
                                break;

                            sensor->xpos     = tsm1 - collisionMasks[player->collisionPlane].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                            sensor->collided = true;
                            break;
                        }
                    }
                }

                if (sensor->collided) {
                    if (sensor->xpos - startX > tsm1) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                    else if (sensor->xpos - startX < -tsm1) {
                        sensor->xpos     = startX << 16;
                        sensor->collided = false;
                    }
                }
            }
        }
    }
}

void RSDK::Legacy::v4::ProcessAirCollision(Entity *entity)
{
    Hitbox *playerHitbox = GetHitbox(entity);
    collisionLeft        = playerHitbox->left[0];
    collisionTop         = playerHitbox->top[0];
    collisionRight       = playerHitbox->right[0];
    collisionBottom      = playerHitbox->bottom[0];

    uint8 movingDown  = 0;
    uint8 movingUp    = 0;
    uint8 movingLeft  = 0;
    uint8 movingRight = 0;

    if (entity->xvel < 0) {
        movingRight = 0;
    }
    else {
        movingRight         = 1;
        sensors[0].ypos     = entity->ypos + 0x40000;
        sensors[0].collided = false;
        sensors[0].xpos     = entity->xpos + (collisionRight << 16);
    }
    if (entity->xvel > 0) {
        movingLeft = 0;
    }
    else {
        movingLeft          = 1;
        sensors[1].ypos     = entity->ypos + 0x40000;
        sensors[1].collided = false;
        sensors[1].xpos     = entity->xpos + ((collisionLeft - 1) << 16);
    }
    sensors[2].xpos     = entity->xpos + (playerHitbox->left[1] << 16);
    sensors[3].xpos     = entity->xpos + (playerHitbox->right[1] << 16);
    sensors[2].collided = false;
    sensors[3].collided = false;
    sensors[4].xpos     = sensors[2].xpos;
    sensors[5].xpos     = sensors[3].xpos;
    sensors[4].collided = false;
    sensors[5].collided = false;
    if (entity->yvel < 0) {
        movingDown = 0;
    }
    else {
        movingDown      = 1;
        sensors[2].ypos = entity->ypos + (collisionBottom << 16);
        sensors[3].ypos = entity->ypos + (collisionBottom << 16);
    }

    if (abs(entity->xvel) > 0x10000 || entity->yvel < 0) {
        movingUp        = 1;
        sensors[4].ypos = entity->ypos + ((collisionTop - 1) << 16);
        sensors[5].ypos = entity->ypos + ((collisionTop - 1) << 16);
    }

    int32 cnt   = (abs(entity->xvel) <= abs(entity->yvel) ? (abs(entity->yvel) >> 19) + 1 : (abs(entity->xvel) >> 19) + 1);
    int32 XVel  = entity->xvel / cnt;
    int32 YVel  = entity->yvel / cnt;
    int32 XVel2 = entity->xvel - XVel * (cnt - 1);
    int32 YVel2 = entity->yvel - YVel * (cnt - 1);
    while (cnt > 0) {
        if (cnt < 2) {
            XVel = XVel2;
            YVel = YVel2;
        }
        cnt--;

        if (movingRight == 1) {
            sensors[0].xpos += XVel;
            sensors[0].ypos += YVel;
            LWallCollision(entity, &sensors[0]);
            if (sensors[0].collided) {
                movingRight = 2;
            }
            else if (entity->xvel < 0x20000) {
                sensors[0].ypos -= 0x80000;
                LWallCollision(entity, &sensors[0]);
                if (sensors[0].collided)
                    movingRight = 2;
                sensors[0].ypos += 0x80000;
            }
        }

        if (movingLeft == 1) {
            sensors[1].xpos += XVel;
            sensors[1].ypos += YVel;
            RWallCollision(entity, &sensors[1]);
            if (sensors[1].collided) {
                movingLeft = 2;
            }
            else if (entity->xvel > -0x20000) {
                sensors[1].ypos -= 0x80000;
                RWallCollision(entity, &sensors[1]);
                if (sensors[1].collided)
                    movingLeft = 2;
                sensors[1].ypos += 0x80000;
            }
        }

        if (movingRight == 2) {
            entity->xvel    = 0;
            entity->speed   = 0;
            entity->xpos    = (sensors[0].xpos - collisionRight) << 16;
            sensors[2].xpos = entity->xpos + ((collisionLeft + 1) << 16);
            sensors[3].xpos = entity->xpos + ((collisionRight - 2) << 16);
            sensors[4].xpos = sensors[2].xpos;
            sensors[5].xpos = sensors[3].xpos;
            XVel            = 0;
            XVel2           = 0;
            movingRight     = 3;
        }

        if (movingLeft == 2) {
            entity->xvel    = 0;
            entity->speed   = 0;
            entity->xpos    = (sensors[1].xpos - collisionLeft + 1) << 16;
            sensors[2].xpos = entity->xpos + ((collisionLeft + 1) << 16);
            sensors[3].xpos = entity->xpos + ((collisionRight - 2) << 16);
            sensors[4].xpos = sensors[2].xpos;
            sensors[5].xpos = sensors[3].xpos;
            XVel            = 0;
            XVel2           = 0;
            movingLeft      = 3;
        }

        if (movingDown == 1) {
            for (int32 i = 2; i < 4; i++) {
                if (!sensors[i].collided) {
                    sensors[i].xpos += XVel;
                    sensors[i].ypos += YVel;
                    FloorCollision(entity, &sensors[i]);
                }
            }
            if (sensors[2].collided || sensors[3].collided) {
                movingDown = 2;
                cnt        = 0;
            }
        }

        if (movingUp == 1) {
            for (int32 i = 4; i < 6; i++) {
                if (!sensors[i].collided) {
                    sensors[i].xpos += XVel;
                    sensors[i].ypos += YVel;
                    RoofCollision(entity, &sensors[i]);
                }
            }
            if (sensors[4].collided || sensors[5].collided) {
                movingUp = 2;
                cnt      = 0;
            }
        }
    }

    if (movingRight < 2 && movingLeft < 2)
        entity->xpos = entity->xpos + entity->xvel;

    if (movingUp < 2 && movingDown < 2) {
        entity->ypos = entity->ypos + entity->yvel;
        return;
    }

    if (movingDown == 2) {
        entity->gravity = 0;
        if (sensors[2].collided && sensors[3].collided) {
            if (sensors[2].ypos >= sensors[3].ypos) {
                entity->ypos  = (sensors[3].ypos - collisionBottom) << 16;
                entity->angle = sensors[3].angle;
            }
            else {
                entity->ypos  = (sensors[2].ypos - collisionBottom) << 16;
                entity->angle = sensors[2].angle;
            }
        }
        else if (sensors[2].collided == 1) {
            entity->ypos  = (sensors[2].ypos - collisionBottom) << 16;
            entity->angle = sensors[2].angle;
        }
        else if (sensors[3].collided == 1) {
            entity->ypos  = (sensors[3].ypos - collisionBottom) << 16;
            entity->angle = sensors[3].angle;
        }
        if (entity->angle > 0xA0 && entity->angle < 0xE0 && entity->collisionMode != CMODE_LWALL) {
            entity->collisionMode = CMODE_LWALL;
            entity->xpos -= 0x40000;
        }
        if (entity->angle > 0x20 && entity->angle < 0x60 && entity->collisionMode != CMODE_RWALL) {
            entity->collisionMode = CMODE_RWALL;
            entity->xpos += 0x40000;
        }
        if (entity->angle < 0x20 || entity->angle > 0xE0) {
            entity->controlLock = 0;
        }
        entity->rotation = entity->angle << 1;

        int32 speed = 0;
        if (entity->down) {
            if (entity->angle < 128) {
                if (entity->angle < 16) {
                    speed = entity->xvel;
                }
                else if (entity->angle >= 32) {
                    speed = (abs(entity->xvel) <= abs(entity->yvel) ? entity->yvel + entity->yvel / 12 : entity->xvel);
                }
                else {
                    speed = (abs(entity->xvel) <= abs(entity->yvel >> 1) ? (entity->yvel + entity->yvel / 12) >> 1 : entity->xvel);
                }
            }
            else if (entity->angle > 240) {
                speed = entity->xvel;
            }
            else if (entity->angle <= 224) {
                speed = (abs(entity->xvel) <= abs(entity->yvel) ? -(entity->yvel + entity->yvel / 12) : entity->xvel);
            }
            else {
                speed = (abs(entity->xvel) <= abs(entity->yvel >> 1) ? -((entity->yvel + entity->yvel / 12) >> 1) : entity->xvel);
            }
        }
        else if (entity->angle < 0x80) {
            if (entity->angle < 0x10) {
                speed = entity->xvel;
            }
            else if (entity->angle >= 0x20) {
                speed = (abs(entity->xvel) <= abs(entity->yvel) ? entity->yvel : entity->xvel);
            }
            else {
                speed = (abs(entity->xvel) <= abs(entity->yvel >> 1) ? entity->yvel >> 1 : entity->xvel);
            }
        }
        else if (entity->angle > 0xF0) {
            speed = entity->xvel;
        }
        else if (entity->angle <= 0xE0) {
            speed = (abs(entity->xvel) <= abs(entity->yvel) ? -entity->yvel : entity->xvel);
        }
        else {
            speed = (abs(entity->xvel) <= abs(entity->yvel >> 1) ? -(entity->yvel >> 1) : entity->xvel);
        }

        if (speed < -0x180000)
            speed = -0x180000;
        if (speed > 0x180000)
            speed = 0x180000;
        entity->speed         = speed;
        entity->yvel          = 0;
        scriptEng.checkResult = 1;
    }

    if (movingUp == 2) {
        int32 sensorAngle = 0;
        if (sensors[4].collided && sensors[5].collided) {
            if (sensors[4].ypos <= sensors[5].ypos) {
                entity->ypos = (sensors[5].ypos - collisionTop + 1) << 16;
                sensorAngle  = sensors[5].angle;
            }
            else {
                entity->ypos = (sensors[4].ypos - collisionTop + 1) << 16;
                sensorAngle  = sensors[4].angle;
            }
        }
        else if (sensors[4].collided) {
            entity->ypos = (sensors[4].ypos - collisionTop + 1) << 16;
            sensorAngle  = sensors[4].angle;
        }
        else if (sensors[5].collided) {
            entity->ypos = (sensors[5].ypos - collisionTop + 1) << 16;
            sensorAngle  = sensors[5].angle;
        }
        sensorAngle &= 0xFF;

        int32 angle = ArcTanLookup(entity->xvel, entity->yvel);
        if (sensorAngle > 0x40 && sensorAngle < 0x62 && angle > 0xA0 && angle < 0xC2) {
            entity->gravity       = 0;
            entity->angle         = sensorAngle;
            entity->rotation      = entity->angle << 1;
            entity->collisionMode = CMODE_RWALL;
            entity->xpos += 0x40000;
            entity->ypos -= 0x20000;
            if (entity->angle <= 0x60)
                entity->speed = entity->yvel;
            else
                entity->speed = entity->yvel >> 1;
        }
        if (sensorAngle > 0x9E && sensorAngle < 0xC0 && angle > 0xBE && angle < 0xE0) {
            entity->gravity       = 0;
            entity->angle         = sensorAngle;
            entity->rotation      = entity->angle << 1;
            entity->collisionMode = CMODE_LWALL;
            entity->xpos -= 0x40000;
            entity->ypos -= 0x20000;
            if (entity->angle >= 0xA0)
                entity->speed = -entity->yvel;
            else
                entity->speed = -entity->yvel >> 1;
        }
        if (entity->yvel < 0)
            entity->yvel = 0;
        scriptEng.checkResult = 2;
    }
}
void RSDK::Legacy::v4::ProcessPathGrip(Entity *entity)
{
    int32 cosValue256;
    int32 sinValue256;
    sensors[4].xpos = entity->xpos;
    sensors[4].ypos = entity->ypos;
    for (int32 i = 0; i < 7; ++i) {
        sensors[i].angle    = entity->angle;
        sensors[i].collided = false;
    }
    SetPathGripSensors(entity);
    int32 absSpeed  = abs(entity->speed);
    int32 checkDist = absSpeed >> 18;
    absSpeed &= 0x3FFFF;
    uint8 cMode = entity->collisionMode;

    while (checkDist > -1) {
        if (checkDist >= 1) {
            cosValue256 = cos256LookupTable[entity->angle] << 10;
            sinValue256 = sin256LookupTable[entity->angle] << 10;
            checkDist--;
        }
        else {
            cosValue256 = absSpeed * cos256LookupTable[entity->angle] >> 8;
            sinValue256 = absSpeed * sin256LookupTable[entity->angle] >> 8;
            checkDist   = -1;
        }

        if (entity->speed < 0) {
            cosValue256 = -cosValue256;
            sinValue256 = -sinValue256;
        }

        sensors[0].collided = false;
        sensors[1].collided = false;
        sensors[2].collided = false;
        sensors[5].collided = false;
        sensors[6].collided = false;
        sensors[4].xpos += cosValue256;
        sensors[4].ypos += sinValue256;
        int32 tileDistance = -1;

        switch (entity->collisionMode) {
            case CMODE_FLOOR: {
                sensors[3].xpos += cosValue256;
                sensors[3].ypos += sinValue256;

                if (entity->speed > 0) {
                    LWallCollision(entity, &sensors[3]);
                    if (sensors[3].collided) {
                        sensors[2].xpos = (sensors[3].xpos - 2) << 16;
                    }
                }

                if (entity->speed < 0) {
                    RWallCollision(entity, &sensors[3]);
                    if (sensors[3].collided) {
                        sensors[0].xpos = (sensors[3].xpos + 2) << 16;
                    }
                }

                if (sensors[3].collided) {
                    cosValue256 = 0;
                    checkDist   = -1;
                }

                for (int32 i = 0; i < 3; i++) {
                    sensors[i].xpos += cosValue256;
                    sensors[i].ypos += sinValue256;
                    FindFloorPosition(entity, &sensors[i], sensors[i].ypos >> 16);
                }

                for (int32 i = 5; i < 7; i++) {
                    sensors[i].xpos += cosValue256;
                    sensors[i].ypos += sinValue256;
                    FindFloorPosition(entity, &sensors[i], sensors[i].ypos >> 16);
                }

                tileDistance = -1;
                for (int32 i = 0; i < 3; i++) {
                    if (tileDistance > -1) {
                        if (sensors[i].collided) {
                            if (sensors[i].ypos < sensors[tileDistance].ypos)
                                tileDistance = i;

                            if (sensors[i].ypos == sensors[tileDistance].ypos && (sensors[i].angle < 0x08 || sensors[i].angle > 0xF8))
                                tileDistance = i;
                        }
                    }
                    else if (sensors[i].collided)
                        tileDistance = i;
                }

                if (tileDistance <= -1) {
                    checkDist = -1;
                }
                else {
                    sensors[0].ypos  = sensors[tileDistance].ypos << 16;
                    sensors[0].angle = sensors[tileDistance].angle;
                    sensors[1].ypos  = sensors[0].ypos;
                    sensors[1].angle = sensors[0].angle;
                    sensors[2].ypos  = sensors[0].ypos;
                    sensors[2].angle = sensors[0].angle;
                    sensors[3].ypos  = sensors[0].ypos - 0x40000;
                    sensors[3].angle = sensors[0].angle;
                    sensors[4].xpos  = sensors[1].xpos;
                    sensors[4].ypos  = sensors[0].ypos - (collisionBottom << 16);
                }

                if (sensors[0].angle < 0xDE && sensors[0].angle > 0x80)
                    entity->collisionMode = CMODE_LWALL;
                if (sensors[0].angle > 0x22 && sensors[0].angle < 0x80)
                    entity->collisionMode = CMODE_RWALL;
                break;
            }
            case CMODE_LWALL: {
                sensors[3].xpos += cosValue256;
                sensors[3].ypos += sinValue256;

                if (entity->speed > 0)
                    RoofCollision(entity, &sensors[3]);

                if (entity->speed < 0)
                    FloorCollision(entity, &sensors[3]);

                if (sensors[3].collided) {
                    sinValue256 = 0;
                    checkDist   = -1;
                }
                for (int32 i = 0; i < 3; i++) {
                    sensors[i].xpos += cosValue256;
                    sensors[i].ypos += sinValue256;
                    FindLWallPosition(entity, &sensors[i], sensors[i].xpos >> 16);
                }

                tileDistance = -1;
                for (int32 i = 0; i < 3; i++) {
                    if (tileDistance > -1) {
                        if (sensors[i].xpos < sensors[tileDistance].xpos && sensors[i].collided) {
                            tileDistance = i;
                        }
                    }
                    else if (sensors[i].collided) {
                        tileDistance = i;
                    }
                }

                if (tileDistance <= -1) {
                    checkDist = -1;
                }
                else {
                    sensors[0].xpos  = sensors[tileDistance].xpos << 16;
                    sensors[0].angle = sensors[tileDistance].angle;
                    sensors[1].xpos  = sensors[0].xpos;
                    sensors[1].angle = sensors[0].angle;
                    sensors[2].xpos  = sensors[0].xpos;
                    sensors[2].angle = sensors[0].angle;
                    sensors[4].ypos  = sensors[1].ypos;
                    sensors[4].xpos  = sensors[1].xpos - (collisionRight << 16);
                }

                if (sensors[0].angle > 0xE2)
                    entity->collisionMode = CMODE_FLOOR;
                if (sensors[0].angle < 0x9E)
                    entity->collisionMode = CMODE_ROOF;
                break;
                break;
            }
            case CMODE_ROOF: {
                sensors[3].xpos += cosValue256;
                sensors[3].ypos += sinValue256;

                if (entity->speed > 0)
                    RWallCollision(entity, &sensors[3]);

                if (entity->speed < 0)
                    LWallCollision(entity, &sensors[3]);

                if (sensors[3].collided) {
                    cosValue256 = 0;
                    checkDist   = -1;
                }
                for (int32 i = 0; i < 3; i++) {
                    sensors[i].xpos += cosValue256;
                    sensors[i].ypos += sinValue256;
                    FindRoofPosition(entity, &sensors[i], sensors[i].ypos >> 16);
                }

                tileDistance = -1;
                for (int32 i = 0; i < 3; i++) {
                    if (tileDistance > -1) {
                        if (sensors[i].ypos > sensors[tileDistance].ypos && sensors[i].collided) {
                            tileDistance = i;
                        }
                    }
                    else if (sensors[i].collided) {
                        tileDistance = i;
                    }
                }

                if (tileDistance <= -1) {
                    checkDist = -1;
                }
                else {
                    sensors[0].ypos  = sensors[tileDistance].ypos << 16;
                    sensors[0].angle = sensors[tileDistance].angle;
                    sensors[1].ypos  = sensors[0].ypos;
                    sensors[1].angle = sensors[0].angle;
                    sensors[2].ypos  = sensors[0].ypos;
                    sensors[2].angle = sensors[0].angle;
                    sensors[3].ypos  = sensors[0].ypos + 0x40000;
                    sensors[3].angle = sensors[0].angle;
                    sensors[4].xpos  = sensors[1].xpos;
                    sensors[4].ypos  = sensors[0].ypos - ((collisionTop - 1) << 16);
                }

                if (sensors[0].angle > 0xA2)
                    entity->collisionMode = CMODE_LWALL;
                if (sensors[0].angle < 0x5E)
                    entity->collisionMode = CMODE_RWALL;
                break;
            }
            case CMODE_RWALL: {
                sensors[3].xpos += cosValue256;
                sensors[3].ypos += sinValue256;

                if (entity->speed > 0)
                    FloorCollision(entity, &sensors[3]);

                if (entity->speed < 0)
                    RoofCollision(entity, &sensors[3]);

                if (sensors[3].collided) {
                    sinValue256 = 0;
                    checkDist   = -1;
                }
                for (int32 i = 0; i < 3; i++) {
                    sensors[i].xpos += cosValue256;
                    sensors[i].ypos += sinValue256;
                    FindRWallPosition(entity, &sensors[i], sensors[i].xpos >> 16);
                }

                tileDistance = -1;
                for (int32 i = 0; i < 3; i++) {
                    if (tileDistance > -1) {
                        if (sensors[i].xpos > sensors[tileDistance].xpos && sensors[i].collided) {
                            tileDistance = i;
                        }
                    }
                    else if (sensors[i].collided) {
                        tileDistance = i;
                    }
                }

                if (tileDistance <= -1) {
                    checkDist = -1;
                }
                else {
                    sensors[0].xpos  = sensors[tileDistance].xpos << 16;
                    sensors[0].angle = sensors[tileDistance].angle;
                    sensors[1].xpos  = sensors[0].xpos;
                    sensors[1].angle = sensors[0].angle;
                    sensors[2].xpos  = sensors[0].xpos;
                    sensors[2].angle = sensors[0].angle;
                    sensors[4].ypos  = sensors[1].ypos;
                    sensors[4].xpos  = sensors[1].xpos - ((collisionLeft - 1) << 16);
                }

                if (sensors[0].angle < 0x1E)
                    entity->collisionMode = CMODE_FLOOR;
                if (sensors[0].angle > 0x62)
                    entity->collisionMode = CMODE_ROOF;
                break;
            }
        }
        if (tileDistance != -1)
            entity->angle = sensors[0].angle;

        if (!sensors[3].collided)
            SetPathGripSensors(entity);
        else
            checkDist = -2;
    }

    switch (cMode) {
        case CMODE_FLOOR: {
            if (sensors[0].collided || sensors[1].collided || sensors[2].collided) {
                entity->angle           = sensors[0].angle;
                entity->rotation        = entity->angle << 1;
                entity->floorSensors[0] = sensors[0].collided;
                entity->floorSensors[1] = sensors[1].collided;
                entity->floorSensors[2] = sensors[2].collided;
                entity->floorSensors[3] = sensors[5].collided;
                entity->floorSensors[4] = sensors[6].collided;

                if (!sensors[3].collided) {
                    entity->pushing = 0;
                    entity->xpos    = sensors[4].xpos;
                }
                else {
                    if (entity->speed > 0)
                        entity->xpos = (sensors[3].xpos - collisionRight) << 16;

                    if (entity->speed < 0)
                        entity->xpos = (sensors[3].xpos - collisionLeft + 1) << 16;

                    entity->speed = 0;
                    if ((entity->left || entity->right) && entity->pushing < 2)
                        entity->pushing++;
                }
                entity->ypos = sensors[4].ypos;
            }
            else {
                entity->gravity       = 1;
                entity->collisionMode = CMODE_FLOOR;
                entity->xvel          = cos256LookupTable[entity->angle] * entity->speed >> 8;
                entity->yvel          = sin256LookupTable[entity->angle] * entity->speed >> 8;
                if (entity->yvel < -0x100000)
                    entity->yvel = -0x100000;

                if (entity->yvel > 0x100000)
                    entity->yvel = 0x100000;

                entity->speed = entity->xvel;
                entity->angle = 0;
                if (!sensors[3].collided) {
                    entity->pushing = 0;
                    entity->xpos += entity->xvel;
                }
                else {
                    if (entity->speed > 0)
                        entity->xpos = (sensors[3].xpos - collisionRight) << 16;
                    if (entity->speed < 0)
                        entity->xpos = (sensors[3].xpos - collisionLeft + 1) << 16;

                    entity->speed = 0;
                    if ((entity->left || entity->right) && entity->pushing < 2)
                        entity->pushing++;
                }
                entity->ypos += entity->yvel;
            }
            break;
        }
        case CMODE_LWALL: {
            if (!sensors[0].collided && !sensors[1].collided && !sensors[2].collided) {
                entity->gravity       = 1;
                entity->collisionMode = CMODE_FLOOR;
                entity->xvel          = cos256LookupTable[entity->angle] * entity->speed >> 8;
                entity->yvel          = sin256LookupTable[entity->angle] * entity->speed >> 8;
                if (entity->yvel < -0x100000) {
                    entity->yvel = -0x100000;
                }
                if (entity->yvel > 0x100000) {
                    entity->yvel = 0x100000;
                }
                entity->speed = entity->xvel;
                entity->angle = 0;
            }
            else if (entity->speed >= 0x28000 || entity->speed <= -0x28000 || entity->controlLock != 0) {
                entity->angle    = sensors[0].angle;
                entity->rotation = entity->angle << 1;
            }
            else {
                entity->gravity       = 1;
                entity->angle         = 0;
                entity->collisionMode = CMODE_FLOOR;
                entity->speed         = entity->xvel;
                entity->controlLock   = 30;
            }
            if (!sensors[3].collided) {
                entity->ypos = sensors[4].ypos;
            }
            else {
                if (entity->speed > 0)
                    entity->ypos = (sensors[3].ypos - collisionTop) << 16;

                if (entity->speed < 0)
                    entity->ypos = (sensors[3].ypos - collisionBottom) << 16;

                entity->speed = 0;
            }
            entity->xpos = sensors[4].xpos;
            break;
        }
        case CMODE_ROOF: {
            if (!sensors[0].collided && !sensors[1].collided && !sensors[2].collided) {
                entity->gravity         = 1;
                entity->collisionMode   = CMODE_FLOOR;
                entity->xvel            = cos256LookupTable[entity->angle] * entity->speed >> 8;
                entity->yvel            = sin256LookupTable[entity->angle] * entity->speed >> 8;
                entity->floorSensors[0] = false;
                entity->floorSensors[1] = false;
                entity->floorSensors[2] = false;
                if (entity->yvel < -0x100000)
                    entity->yvel = -0x100000;

                if (entity->yvel > 0x100000)
                    entity->yvel = 0x100000;

                entity->angle = 0;
                entity->speed = entity->xvel;
                if (!sensors[3].collided) {
                    entity->xpos = entity->xpos + entity->xvel;
                }
                else {
                    if (entity->speed > 0)
                        entity->xpos = (sensors[3].xpos - collisionRight) << 16;

                    if (entity->speed < 0)
                        entity->xpos = (sensors[3].xpos - collisionLeft + 1) << 16;

                    entity->speed = 0;
                }
            }
            else if (entity->speed <= -0x28000 || entity->speed >= 0x28000) {
                entity->angle    = sensors[0].angle;
                entity->rotation = entity->angle << 1;
                if (!sensors[3].collided) {
                    entity->xpos = sensors[4].xpos;
                }
                else {
                    if (entity->speed < 0)
                        entity->xpos = (sensors[3].xpos - collisionRight) << 16;

                    if (entity->speed > 0)
                        entity->xpos = (sensors[3].xpos - collisionLeft + 1) << 16;
                    entity->speed = 0;
                }
            }
            else {
                entity->gravity         = 1;
                entity->angle           = 0;
                entity->collisionMode   = CMODE_FLOOR;
                entity->speed           = entity->xvel;
                entity->floorSensors[0] = false;
                entity->floorSensors[1] = false;
                entity->floorSensors[2] = false;
                if (!sensors[3].collided) {
                    entity->xpos = entity->xpos + entity->xvel;
                }
                else {
                    if (entity->speed > 0)
                        entity->xpos = (sensors[3].xpos - collisionRight) << 16;

                    if (entity->speed < 0)
                        entity->xpos = (sensors[3].xpos - collisionLeft + 1) << 16;
                    entity->speed = 0;
                }
            }
            entity->ypos = sensors[4].ypos;
            break;
        }
        case CMODE_RWALL: {
            if (!sensors[0].collided && !sensors[1].collided && !sensors[2].collided) {
                entity->gravity       = 1;
                entity->collisionMode = CMODE_FLOOR;
                entity->xvel          = cos256LookupTable[entity->angle] * entity->speed >> 8;
                entity->yvel          = sin256LookupTable[entity->angle] * entity->speed >> 8;
                if (entity->yvel < -0x100000)
                    entity->yvel = -0x100000;

                if (entity->yvel > 0x100000)
                    entity->yvel = 0x100000;

                entity->speed = entity->xvel;
                entity->angle = 0;
            }
            else if (entity->speed <= -0x28000 || entity->speed >= 0x28000 || entity->controlLock != 0) {
                entity->angle    = sensors[0].angle;
                entity->rotation = entity->angle << 1;
            }
            else {
                entity->gravity       = 1;
                entity->angle         = 0;
                entity->collisionMode = CMODE_FLOOR;
                entity->speed         = entity->xvel;
                entity->controlLock   = 30;
            }
            if (!sensors[3].collided) {
                entity->ypos = sensors[4].ypos;
            }
            else {
                if (entity->speed > 0)
                    entity->ypos = (sensors[3].ypos - collisionBottom) << 16;

                if (entity->speed < 0)
                    entity->ypos = (sensors[3].ypos - collisionTop + 1) << 16;

                entity->speed = 0;
            }
            entity->xpos = sensors[4].xpos;
            break;
        }
        default: break;
    }
}

void RSDK::Legacy::v4::SetPathGripSensors(Entity *player)
{
    Hitbox *playerHitbox = GetHitbox(player);

    switch (player->collisionMode) {
        case CMODE_FLOOR: {
            collisionLeft   = playerHitbox->left[0];
            collisionTop    = playerHitbox->top[0];
            collisionRight  = playerHitbox->right[0];
            collisionBottom = playerHitbox->bottom[0];
            sensors[0].ypos = sensors[4].ypos + (collisionBottom << 16);
            sensors[1].ypos = sensors[0].ypos;
            sensors[2].ypos = sensors[0].ypos;
            sensors[3].ypos = sensors[4].ypos + 0x40000;
            sensors[5].ypos = sensors[0].ypos;
            sensors[6].ypos = sensors[0].ypos;

            sensors[0].xpos = sensors[4].xpos + ((playerHitbox->left[1] - 1) << 16);
            sensors[1].xpos = sensors[4].xpos;
            sensors[2].xpos = sensors[4].xpos + (playerHitbox->right[1] << 16);
            sensors[5].xpos = sensors[4].xpos + (playerHitbox->left[1] << 15);
            sensors[6].xpos = sensors[4].xpos + (playerHitbox->right[1] << 15);

            if (player->speed > 0) {
                sensors[3].xpos = sensors[4].xpos + ((collisionRight + 1) << 16);
            }
            else {
                sensors[3].xpos = sensors[4].xpos + ((collisionLeft - 1) << 16);
            }
            return;
        }
        case CMODE_LWALL: {
            collisionLeft   = playerHitbox->left[2];
            collisionTop    = playerHitbox->top[2];
            collisionRight  = playerHitbox->right[2];
            collisionBottom = playerHitbox->bottom[2];
            sensors[0].xpos = sensors[4].xpos + (collisionRight << 16);
            sensors[1].xpos = sensors[0].xpos;
            sensors[2].xpos = sensors[0].xpos;
            sensors[3].xpos = sensors[4].xpos + 0x40000;
            sensors[0].ypos = sensors[4].ypos + ((playerHitbox->top[3] - 1) << 16);
            sensors[1].ypos = sensors[4].ypos;
            sensors[2].ypos = sensors[4].ypos + (playerHitbox->bottom[3] << 16);
            if (player->speed > 0) {
                sensors[3].ypos = sensors[4].ypos + (collisionTop << 16);
            }
            else {
                sensors[3].ypos = sensors[4].ypos + ((collisionBottom - 1) << 16);
            }
            return;
        }
        case CMODE_ROOF: {
            collisionLeft   = playerHitbox->left[4];
            collisionTop    = playerHitbox->top[4];
            collisionRight  = playerHitbox->right[4];
            collisionBottom = playerHitbox->bottom[4];
            sensors[0].ypos = sensors[4].ypos + ((collisionTop - 1) << 16);
            sensors[1].ypos = sensors[0].ypos;
            sensors[2].ypos = sensors[0].ypos;
            sensors[3].ypos = sensors[4].ypos - 0x40000;
            sensors[0].xpos = sensors[4].xpos + ((playerHitbox->left[5] - 1) << 16);
            sensors[1].xpos = sensors[4].xpos;
            sensors[2].xpos = sensors[4].xpos + (playerHitbox->right[5] << 16);
            if (player->speed < 0) {
                sensors[3].xpos = sensors[4].xpos + ((collisionRight + 1) << 16);
            }
            else {
                sensors[3].xpos = sensors[4].xpos + ((collisionLeft - 1) << 16);
            }
            return;
        }
        case CMODE_RWALL: {
            collisionLeft   = playerHitbox->left[6];
            collisionTop    = playerHitbox->top[6];
            collisionRight  = playerHitbox->right[6];
            collisionBottom = playerHitbox->bottom[6];
            sensors[0].xpos = sensors[4].xpos + ((collisionLeft - 1) << 16);
            sensors[1].xpos = sensors[0].xpos;
            sensors[2].xpos = sensors[0].xpos;
            sensors[3].xpos = sensors[4].xpos - 0x40000;
            sensors[0].ypos = sensors[4].ypos + ((playerHitbox->top[7] - 1) << 16);
            sensors[1].ypos = sensors[4].ypos;
            sensors[2].ypos = sensors[4].ypos + (playerHitbox->bottom[7] << 16);
            if (player->speed > 0) {
                sensors[3].ypos = sensors[4].ypos + (collisionBottom << 16);
            }
            else {
                sensors[3].ypos = sensors[4].ypos + ((collisionTop - 1) << 16);
            }
            return;
        }
        default: return;
    }
}

void RSDK::Legacy::v4::ProcessTileCollisions(Entity *player)
{
    player->floorSensors[0] = false;
    player->floorSensors[1] = false;
    player->floorSensors[2] = false;
    player->floorSensors[3] = false;
    player->floorSensors[4] = false;

    scriptEng.checkResult = false;

    collisionTolerance = 15;
    if (player->speed < 0x60000)
        collisionTolerance = player->angle == 0 ? 8 : 15;

    if (player->gravity == 1)
        ProcessAirCollision(player);
    else
        ProcessPathGrip(player);
}

void RSDK::Legacy::v4::ObjectFloorCollision(int32 xOffset, int32 yOffset, int32 cPath)
{
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 c               = 0;
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7) {
        int32 chunkX    = XPos >> 7;
        int32 tileX     = (XPos & 0x7F) >> 4;
        int32 chunkY    = YPos >> 7;
        int32 tileY     = (YPos & 0x7F) >> 4;
        int32 chunk     = (stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6) + tileX + (tileY << 3);
        int32 tileIndex = tiles128x128.tileIndex[chunk];
        if (tiles128x128.collisionFlags[cPath][chunk] != SOLID_LRB && tiles128x128.collisionFlags[cPath][chunk] != SOLID_NONE) {
            switch (tiles128x128.direction[chunk]) {
                case 0: {
                    c = (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) <= collisionMasks[cPath].floorMasks[c]) {
                        break;
                    }
                    YPos                  = collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 1: {
                    c = 15 - (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) <= collisionMasks[cPath].floorMasks[c]) {
                        break;
                    }
                    YPos                  = collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 2: {
                    c = (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) <= 15 - collisionMasks[cPath].roofMasks[c]) {
                        break;
                    }
                    YPos                  = 15 - collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 3: {
                    c = 15 - (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) <= 15 - collisionMasks[cPath].roofMasks[c]) {
                        break;
                    }
                    YPos                  = 15 - collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
            }
        }
        if (scriptEng.checkResult) {
            entity->ypos = (YPos - yOffset) << 16;
        }
    }
}
void RSDK::Legacy::v4::ObjectLWallCollision(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7) {
        int32 chunkX    = XPos >> 7;
        int32 tileX     = (XPos & 0x7F) >> 4;
        int32 chunkY    = YPos >> 7;
        int32 tileY     = (YPos & 0x7F) >> 4;
        int32 chunk     = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
        chunk           = chunk + tileX + (tileY << 3);
        int32 tileIndex = tiles128x128.tileIndex[chunk];
        if (tiles128x128.collisionFlags[cPath][chunk] != SOLID_TOP && tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
            switch (tiles128x128.direction[chunk]) {
                case 0: {
                    c = (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) <= collisionMasks[cPath].lWallMasks[c]) {
                        break;
                    }
                    XPos                  = collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 1: {
                    c = (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) <= 15 - collisionMasks[cPath].rWallMasks[c]) {
                        break;
                    }
                    XPos                  = 15 - collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 2: {
                    c = 15 - (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) <= collisionMasks[cPath].lWallMasks[c]) {
                        break;
                    }
                    XPos                  = collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 3: {
                    c = 15 - (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) <= 15 - collisionMasks[cPath].rWallMasks[c]) {
                        break;
                    }
                    XPos                  = 15 - collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
            }
        }
        if (scriptEng.checkResult) {
            entity->xpos = (XPos - xOffset) << 16;
        }
    }
}
void RSDK::Legacy::v4::ObjectRoofCollision(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7) {
        int32 chunkX    = XPos >> 7;
        int32 tileX     = (XPos & 0x7F) >> 4;
        int32 chunkY    = YPos >> 7;
        int32 tileY     = (YPos & 0x7F) >> 4;
        int32 chunk     = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
        chunk           = chunk + tileX + (tileY << 3);
        int32 tileIndex = tiles128x128.tileIndex[chunk];
        if (tiles128x128.collisionFlags[cPath][chunk] != SOLID_TOP && tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
            switch (tiles128x128.direction[chunk]) {
                case 0: {
                    c = (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) >= collisionMasks[cPath].roofMasks[c]) {
                        break;
                    }
                    YPos                  = collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 1: {
                    c = 15 - (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) >= collisionMasks[cPath].roofMasks[c]) {
                        break;
                    }
                    YPos                  = collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 2: {
                    c = (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) >= 15 - collisionMasks[cPath].floorMasks[c]) {
                        break;
                    }
                    YPos                  = 15 - collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 3: {
                    c = 15 - (XPos & 15) + (tileIndex << 4);
                    if ((YPos & 15) >= 15 - collisionMasks[cPath].floorMasks[c]) {
                        break;
                    }
                    YPos                  = 15 - collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                    scriptEng.checkResult = true;
                    break;
                }
            }
        }
        if (scriptEng.checkResult) {
            entity->ypos = (YPos - yOffset) << 16;
        }
    }
}
void RSDK::Legacy::v4::ObjectRWallCollision(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7) {
        int32 chunkX    = XPos >> 7;
        int32 tileX     = (XPos & 0x7F) >> 4;
        int32 chunkY    = YPos >> 7;
        int32 tileY     = (YPos & 0x7F) >> 4;
        int32 chunk     = stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6;
        chunk           = chunk + tileX + (tileY << 3);
        int32 tileIndex = tiles128x128.tileIndex[chunk];
        if (tiles128x128.collisionFlags[cPath][chunk] != SOLID_TOP && tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
            switch (tiles128x128.direction[chunk]) {
                case 0: {
                    c = (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) >= collisionMasks[cPath].rWallMasks[c]) {
                        break;
                    }
                    XPos                  = collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 1: {
                    c = (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) >= 15 - collisionMasks[cPath].lWallMasks[c]) {
                        break;
                    }
                    XPos                  = 15 - collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 2: {
                    c = 15 - (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) >= collisionMasks[cPath].rWallMasks[c]) {
                        break;
                    }
                    XPos                  = collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
                case 3: {
                    c = 15 - (YPos & 15) + (tileIndex << 4);
                    if ((XPos & 15) >= 15 - collisionMasks[cPath].lWallMasks[c]) {
                        break;
                    }
                    XPos                  = 15 - collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                    scriptEng.checkResult = true;
                    break;
                }
            }
        }
        if (scriptEng.checkResult) {
            entity->xpos = (XPos - xOffset) << 16;
        }
    }
}

void RSDK::Legacy::v4::ObjectFloorGrip(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 chunkX          = YPos;
    YPos                  = YPos - 16;
    for (int32 i = 3; i > 0; i--) {
        if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7 && !scriptEng.checkResult) {
            int32 chunkX    = XPos >> 7;
            int32 tileX     = (XPos & 0x7F) >> 4;
            int32 chunkY    = YPos >> 7;
            int32 tileY     = (YPos & 0x7F) >> 4;
            int32 chunk     = (stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6) + tileX + (tileY << 3);
            int32 tileIndex = tiles128x128.tileIndex[chunk];
            if (tiles128x128.collisionFlags[cPath][chunk] != SOLID_LRB && tiles128x128.collisionFlags[cPath][chunk] != SOLID_NONE) {
                switch (tiles128x128.direction[chunk]) {
                    case 0: {
                        c = (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].floorMasks[c] >= 64) {
                            break;
                        }
                        entity->ypos          = collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 1: {
                        c = 15 - (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].floorMasks[c] >= 64) {
                            break;
                        }
                        entity->ypos          = collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 2: {
                        c = (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].roofMasks[c] <= -64) {
                            break;
                        }
                        entity->ypos          = 15 - collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 3: {
                        c = 15 - (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].roofMasks[c] <= -64) {
                            break;
                        }
                        entity->ypos          = 15 - collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                }
            }
        }
        YPos += 16;
    }

    if (scriptEng.checkResult) {
        if (abs(entity->ypos - chunkX) < 16) {
            entity->ypos = (entity->ypos - yOffset) << 16;
            return;
        }
        entity->ypos          = (chunkX - yOffset) << 16;
        scriptEng.checkResult = false;
    }
}
void RSDK::Legacy::v4::ObjectLWallGrip(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 startX          = XPos;
    XPos                  = XPos - 16;
    for (int32 i = 3; i > 0; i--) {
        if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7 && !scriptEng.checkResult) {
            int32 chunkX    = XPos >> 7;
            int32 tileX     = (XPos & 0x7F) >> 4;
            int32 chunkY    = YPos >> 7;
            int32 tileY     = (YPos & 0x7F) >> 4;
            int32 chunk     = (stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6) + tileX + (tileY << 3);
            int32 tileIndex = tiles128x128.tileIndex[chunk];
            if (tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
                switch (tiles128x128.direction[chunk]) {
                    case 0: {
                        c = (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].lWallMasks[c] >= 64) {
                            break;
                        }
                        entity->xpos          = collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 1: {
                        c = (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].rWallMasks[c] <= -64) {
                            break;
                        }
                        entity->xpos          = 15 - collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 2: {
                        c = 15 - (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].lWallMasks[c] >= 64) {
                            break;
                        }
                        entity->xpos          = collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 3: {
                        c = 15 - (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].rWallMasks[c] <= -64) {
                            break;
                        }
                        entity->xpos          = 15 - collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                }
            }
        }
        XPos += 16;
    }
    if (scriptEng.checkResult) {
        if (abs(entity->xpos - startX) < 16) {
            entity->xpos = (entity->xpos - xOffset) << 16;
            return;
        }
        entity->xpos          = (startX - xOffset) << 16;
        scriptEng.checkResult = false;
    }
}
void RSDK::Legacy::v4::ObjectRoofGrip(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 startY          = YPos;
    YPos                  = YPos + 16;
    for (int32 i = 3; i > 0; i--) {
        if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7 && !scriptEng.checkResult) {
            int32 chunkX    = XPos >> 7;
            int32 tileX     = (XPos & 0x7F) >> 4;
            int32 chunkY    = YPos >> 7;
            int32 tileY     = (YPos & 0x7F) >> 4;
            int32 chunk     = (stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6) + tileX + (tileY << 3);
            int32 tileIndex = tiles128x128.tileIndex[chunk];
            if (tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
                switch (tiles128x128.direction[chunk]) {
                    case 0: {
                        c = (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].roofMasks[c] <= -64) {
                            break;
                        }
                        entity->ypos          = collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 1: {
                        c = 15 - (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].roofMasks[c] <= -64) {
                            break;
                        }
                        entity->ypos          = collisionMasks[cPath].roofMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 2: {
                        c = (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].floorMasks[c] >= 64) {
                            break;
                        }
                        entity->ypos          = 15 - collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 3: {
                        c = 15 - (XPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].floorMasks[c] >= 64) {
                            break;
                        }
                        entity->ypos          = 15 - collisionMasks[cPath].floorMasks[c] + (chunkY << 7) + (tileY << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                }
            }
        }
        YPos -= 16;
    }
    if (scriptEng.checkResult) {
        if (abs(entity->ypos - startY) < 16) {
            entity->ypos = (entity->ypos - yOffset) << 16;
            return;
        }
        entity->ypos          = (startY - yOffset) << 16;
        scriptEng.checkResult = false;
    }
}
void RSDK::Legacy::v4::ObjectRWallGrip(int32 xOffset, int32 yOffset, int32 cPath)
{
    int32 c;
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 XPos            = (entity->xpos >> 16) + xOffset;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 startX          = XPos;
    XPos                  = XPos + 16;
    for (int32 i = 3; i > 0; i--) {
        if (XPos > 0 && XPos < stageLayouts[0].xsize << 7 && YPos > 0 && YPos < stageLayouts[0].ysize << 7 && !scriptEng.checkResult) {
            int32 chunkX    = XPos >> 7;
            int32 tileX     = (XPos & 0x7F) >> 4;
            int32 chunkY    = YPos >> 7;
            int32 tileY     = (YPos & 0x7F) >> 4;
            int32 chunk     = (stageLayouts[0].tiles[chunkX + (chunkY << 8)] << 6) + tileX + (tileY << 3);
            int32 tileIndex = tiles128x128.tileIndex[chunk];
            if (tiles128x128.collisionFlags[cPath][chunk] < SOLID_NONE) {
                switch (tiles128x128.direction[chunk]) {
                    case 0: {
                        c = (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].rWallMasks[c] <= -64) {
                            break;
                        }
                        entity->xpos          = collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 1: {
                        c = (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].lWallMasks[c] >= 64) {
                            break;
                        }
                        entity->xpos          = 15 - collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 2: {
                        c = 15 - (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].rWallMasks[c] <= -64) {
                            break;
                        }
                        entity->xpos          = collisionMasks[cPath].rWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                    case 3: {
                        c = 15 - (YPos & 15) + (tileIndex << 4);
                        if (collisionMasks[cPath].lWallMasks[c] >= 64) {
                            break;
                        }
                        entity->xpos          = 15 - collisionMasks[cPath].lWallMasks[c] + (chunkX << 7) + (tileX << 4);
                        scriptEng.checkResult = true;
                        break;
                    }
                }
            }
        }
        XPos -= 16;
    }
    if (scriptEng.checkResult) {
        if (abs(entity->xpos - startX) < 16) {
            entity->xpos = (entity->xpos - xOffset) << 16;
            return;
        }
        entity->xpos          = (startX - xOffset) << 16;
        scriptEng.checkResult = false;
    }
}

void RSDK::Legacy::v4::ObjectLEntityGrip(int32 xOffset, int32 yOffset, int32 cPath) { 
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 mBlockID        = entity->values[44];
    int32 XPos            = (entity->xpos >> 16) + xOffset - 16;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 check           = 0;
    if (mBlockID > 0 && objectTypeGroupList[mBlockID].listSize > 0) {
        TypeGroupList *mBlockGroupList = &objectTypeGroupList[mBlockID];
        for (int32 i = 0; i < objectTypeGroupList[mBlockID].listSize; i++) {
            short entRef                   = mBlockGroupList->entityRefs[i];
            Entity *otherEntity            = &objectEntityList[entRef];
            int32 XPos2                    = otherEntity->xpos >> 16;
            int32 YPos2                    = otherEntity->ypos >> 16;
            if (((((XPos2 - 16) <= XPos) && (XPos <= (XPos2 + 16))) && ((YPos2 - 16) <= YPos)) && (YPos <= (YPos2 + 16))) {
                entity->xpos = otherEntity->xpos - (xOffset << 16) - 0x100000;
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        check                 = 1;
                        scriptEng.checkResult = check;
                    }
                }
            }
            if ((((XPos2 - 16) <= (XPos + 16) && ((XPos + 16) <= (XPos2 + 16))) && (YPos2 - 16) <= YPos) && (YPos <= (YPos2 + 16))) {
                entity->xpos = otherEntity->xpos - (xOffset << 16) - 0x100000;
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        scriptEng.checkResult = check;
                    }
                }
            }

            if (((XPos2 <= (XPos + 32)) && ((XPos + 32) <= (XPos2 + 16))) && (((YPos2 - 16) <= YPos && YPos <= (YPos2 + 16)))) {
                entity->xpos = otherEntity->xpos - (xOffset << 16) - 0x100000;
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        check                 = 1;
                        scriptEng.checkResult = check;
                    }
                }
            }

            if (check != 0) {
                return;
            }
        }
    }
    ObjectLWallGrip(xOffset, yOffset, cPath);
}

void RSDK::Legacy::v4::ObjectREntityGrip(int32 xOffset, int32 yOffset, int32 cPath)
{
    scriptEng.checkResult = false;
    Entity *entity        = &objectEntityList[objectEntityPos];
    int32 mBlockID        = entity->values[44];
    int32 XPos            = (entity->xpos >> 16) + xOffset + 16;
    int32 YPos            = (entity->ypos >> 16) + yOffset;
    int32 check           = 0;
    if (mBlockID > 0 && objectTypeGroupList[mBlockID].listSize > 0) {
        TypeGroupList *mBlockGroupList = &objectTypeGroupList[mBlockID];
        for (int32 i = 0; i < objectTypeGroupList[mBlockID].listSize; i++) {
            short entRef    = mBlockGroupList->entityRefs[i];
            Entity *otherEntity = &objectEntityList[entRef];
            int32 XPos2         = otherEntity->xpos >> 16;
            int32 YPos2         = otherEntity->ypos >> 16;
            if (((((XPos2 - 16) <= XPos) && (XPos <= (XPos2 + 16))) && ((YPos2 - 16) <= YPos)) && (YPos <= (YPos2 + 16))) {
                entity->xpos = otherEntity->xpos + ((16 - xOffset) << 16);
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        check                 = 1;
                        scriptEng.checkResult = check;
                    }
                }
            }
            if ((((XPos2 - 16) <= (XPos + 16) && ((XPos - 16) <= (XPos2 + 16))) && (YPos2 - 16) <= YPos) && (YPos <= (YPos2 + 16))) {
                entity->xpos = otherEntity->xpos + ((16 - xOffset) << 16);
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        scriptEng.checkResult = check;
                    }
                }
            }

            if (((XPos2 <= (XPos - 32)) && ((XPos - 32) <= (XPos2 + 16))) && (((YPos2 - 16) <= YPos && YPos <= (YPos2 + 16)))) {
                entity->xpos = otherEntity->xpos + ((16 - xOffset) << 16);
                if (otherEntity->values[0] == 0) {
                    check                 = 2;
                    scriptEng.checkResult = check;
                }
                else {
                    scriptEng.checkResult = check;
                    if (check != 2) {
                        check                 = 1;
                        scriptEng.checkResult = check;
                    }
                }
            }

            if (check != 0) {
                return;
            }
        }
    }
    ObjectRWallGrip(xOffset, yOffset, cPath);
}

void RSDK::Legacy::v4::TouchCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity,
                                      int32 otherLeft, int32 otherTop, int32 otherRight, int32 otherBottom)
{
    Hitbox *thisHitbox  = GetHitbox(thisEntity);
    Hitbox *otherHitbox = GetHitbox(otherEntity);

    if (thisLeft == C_BOX)
        thisLeft = thisHitbox->left[0];

    if (thisTop == C_BOX)
        thisTop = thisHitbox->top[0];

    if (thisRight == C_BOX)
        thisRight = thisHitbox->right[0];

    if (thisBottom == C_BOX)
        thisBottom = thisHitbox->bottom[0];

    if (otherLeft == C_BOX)
        otherLeft = otherHitbox->left[0];

    if (otherTop == C_BOX)
        otherTop = otherHitbox->top[0];

    if (otherRight == C_BOX)
        otherRight = otherHitbox->right[0];

    if (otherBottom == C_BOX)
        otherBottom = otherHitbox->bottom[0];

#if !RETRO_USE_ORIGINAL_CODE
    int32 thisHitboxID  = 0;
    int32 otherHitboxID = 0;
    if (showHitboxes) {
        thisHitboxID  = AddDebugHitbox(H_TYPE_TOUCH, thisEntity, thisLeft, thisTop, thisRight, thisBottom);
        otherHitboxID = AddDebugHitbox(H_TYPE_TOUCH, otherEntity, otherLeft, otherTop, otherRight, otherBottom);
    }
#endif

    thisLeft += thisEntity->xpos >> 16;
    thisTop += thisEntity->ypos >> 16;
    thisRight += thisEntity->xpos >> 16;
    thisBottom += thisEntity->ypos >> 16;

    otherLeft += otherEntity->xpos >> 16;
    otherTop += otherEntity->ypos >> 16;
    otherRight += otherEntity->xpos >> 16;
    otherBottom += otherEntity->ypos >> 16;

    scriptEng.checkResult = otherRight > thisLeft && otherLeft < thisRight && otherBottom > thisTop && otherTop < thisBottom;

#if !RETRO_USE_ORIGINAL_CODE
    if (showHitboxes) {
        if (thisHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[thisHitboxID].collision |= 1;
        if (otherHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[otherHitboxID].collision |= 1;
    }
#endif
}
void RSDK::Legacy::v4::BoxCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity,
                                    int32 otherLeft, int32 otherTop, int32 otherRight, int32 otherBottom)
{
    Hitbox *thisHitbox  = GetHitbox(thisEntity);
    Hitbox *otherHitbox = GetHitbox(otherEntity);

    if (thisLeft == C_BOX)
        thisLeft = thisHitbox->left[0];

    if (thisTop == C_BOX)
        thisTop = thisHitbox->top[0];

    if (thisRight == C_BOX)
        thisRight = thisHitbox->right[0];

    if (thisBottom == C_BOX)
        thisBottom = thisHitbox->bottom[0];

    if (otherLeft == C_BOX)
        otherLeft = otherHitbox->left[0];

    if (otherTop == C_BOX)
        otherTop = otherHitbox->top[0];

    if (otherRight == C_BOX)
        otherRight = otherHitbox->right[0];

    if (otherBottom == C_BOX)
        otherBottom = otherHitbox->bottom[0];

#if !RETRO_USE_ORIGINAL_CODE
    int32 thisHitboxID  = 0;
    int32 otherHitboxID = 0;
    if (showHitboxes) {
        thisHitboxID  = AddDebugHitbox(H_TYPE_BOX, thisEntity, thisLeft, thisTop, thisRight, thisBottom);
        otherHitboxID = AddDebugHitbox(H_TYPE_BOX, otherEntity, otherLeft, otherTop, otherRight, otherBottom);
    }
#endif

    thisLeft += thisEntity->xpos >> 16;
    thisTop += thisEntity->ypos >> 16;
    thisRight += thisEntity->xpos >> 16;
    thisBottom += thisEntity->ypos >> 16;

    thisLeft <<= 16;
    thisTop <<= 16;
    thisRight <<= 16;
    thisBottom <<= 16;

    otherLeft <<= 16;
    otherTop <<= 16;
    otherRight <<= 16;
    otherBottom <<= 16;

    scriptEng.checkResult = 0;

    int32 rx = otherEntity->xpos >> 16 << 16;
    int32 ry = otherEntity->ypos >> 16 << 16;

    int32 xDif = otherEntity->xpos - thisRight;
    if (thisEntity->xpos > otherEntity->xpos)
        xDif = thisLeft - otherEntity->xpos;
    int32 yDif = thisTop - otherEntity->ypos;
    if (thisEntity->ypos <= otherEntity->ypos)
        yDif = otherEntity->ypos - thisBottom;

    if (xDif <= yDif && abs(otherEntity->xvel) >> 1 <= abs(otherEntity->yvel)) {
        sensors[0].collided = false;
        sensors[1].collided = false;
        sensors[2].collided = false;
        sensors[3].collided = false;
        sensors[4].collided = false;
        sensors[0].xpos     = rx + otherLeft + 0x20000;
        sensors[1].xpos     = rx;
        sensors[2].xpos     = rx + otherRight - 0x20000;
        sensors[3].xpos     = (sensors[0].xpos + rx) >> 1;
        sensors[4].xpos     = (sensors[2].xpos + rx) >> 1;

        sensors[0].ypos = ry + otherBottom;

        if (otherEntity->yvel >= 0) {
            for (int32 i = 0; i < 5; ++i) {
                if (thisLeft < sensors[i].xpos && thisRight > sensors[i].xpos && thisTop <= sensors[0].ypos
                    && thisTop > otherEntity->ypos - otherEntity->yvel) {
                    sensors[i].collided          = true;
                    otherEntity->floorSensors[i] = true;
                }
            }
        }

        if (sensors[0].collided || sensors[1].collided || sensors[2].collided) {
            if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                otherEntity->xvel  = 0;
                otherEntity->speed = 0;
            }
            otherEntity->ypos        = thisTop - otherBottom;
            otherEntity->gravity     = 0;
            otherEntity->yvel        = 0;
            otherEntity->angle       = 0;
            otherEntity->rotation    = 0;
            otherEntity->controlLock = 0;
            scriptEng.checkResult    = 1;
        }
        else {
            sensors[0].collided = false;
            sensors[1].collided = false;
            sensors[0].xpos     = rx + otherLeft + 0x20000;
            sensors[1].xpos     = rx + otherRight - 0x20000;

            sensors[0].ypos = ry + otherTop;

            for (int32 i = 0; i < 2; ++i) {
                if (thisLeft < sensors[1].xpos && thisRight > sensors[0].xpos && thisBottom > sensors[0].ypos
                    && thisBottom < otherEntity->ypos - otherEntity->yvel) {
                    sensors[i].collided = true;
                }
            }

            if (sensors[1].collided || sensors[0].collided) {
                if (otherEntity->gravity == 1)
                    otherEntity->ypos = thisBottom - otherTop;

                if (otherEntity->yvel <= 0)
                    otherEntity->yvel = 0;
                scriptEng.checkResult = 4;
            }
            else {
                sensors[0].collided = false;
                sensors[1].collided = false;
                sensors[0].xpos     = rx + otherRight;

                sensors[0].ypos = ry + otherTop + 0x20000;
                sensors[1].ypos = ry + otherBottom - 0x20000;
                for (int32 i = 0; i < 2; ++i) {
                    if (thisLeft <= sensors[0].xpos && thisLeft > otherEntity->xpos - otherEntity->xvel && thisTop < sensors[1].ypos
                        && thisBottom > sensors[0].ypos) {
                        sensors[i].collided = true;
                    }
                }

                if (sensors[1].collided || sensors[0].collided) {
                    otherEntity->xpos = thisLeft - otherRight;
                    if (otherEntity->xvel > 0) {
                        if (!otherEntity->direction)
                            otherEntity->pushing = 2;

                        otherEntity->xvel = 0;
                        if (otherEntity->collisionMode || !otherEntity->left)
                            otherEntity->speed = 0;
                        else
                            otherEntity->speed = -0x8000;
                    }
                    scriptEng.checkResult = 2;
                }
                else {
                    sensors[0].collided = false;
                    sensors[1].collided = false;
                    sensors[0].xpos     = rx + otherLeft;

                    sensors[0].ypos = ry + otherTop + 0x20000;
                    sensors[1].ypos = ry + otherBottom - 0x20000;
                    for (int32 i = 0; i < 2; ++i) {
                        if (thisRight > sensors[0].xpos && thisRight < otherEntity->xpos - otherEntity->xvel && thisTop < sensors[1].ypos
                            && thisBottom > sensors[0].ypos) {
                            sensors[i].collided = true;
                        }
                    }

                    if (sensors[1].collided || sensors[0].collided) {
                        otherEntity->xpos = thisRight - otherLeft;
                        if (otherEntity->xvel < 0) {
                            if (otherEntity->direction == FLIP_X)
                                otherEntity->pushing = 2;

                            if (otherEntity->xvel < -0x10000)
                                otherEntity->xpos += 0x8000;

                            otherEntity->xvel = 0;
                            if (otherEntity->collisionMode || !otherEntity->right)
                                otherEntity->speed = 0;
                            else
                                otherEntity->speed = 0x8000;
                        }
                        scriptEng.checkResult = 3;
                    }
                }
            }
        }
    }
    else {
        sensors[0].collided = false;
        sensors[1].collided = false;
        sensors[0].xpos     = rx + otherRight;

        sensors[0].ypos = ry + otherTop + 0x20000;
        sensors[1].ypos = ry + otherBottom - 0x20000;
        for (int32 i = 0; i < 2; ++i) {
            if (thisLeft <= sensors[0].xpos && thisLeft > otherEntity->xpos - otherEntity->xvel && thisTop < sensors[1].ypos
                && thisBottom > sensors[0].ypos) {
                sensors[i].collided = true;
            }
        }
        if (sensors[1].collided || sensors[0].collided) {
            otherEntity->xpos = thisLeft - otherRight;
            if (otherEntity->xvel > 0) {
                if (!otherEntity->direction)
                    otherEntity->pushing = 2;

                otherEntity->xvel = 0;
                if (otherEntity->collisionMode || !otherEntity->left)
                    otherEntity->speed = 0;
                else
                    otherEntity->speed = -0x8000;
            }
            scriptEng.checkResult = 2;
        }
        else {
            sensors[0].collided = false;
            sensors[1].collided = false;
            sensors[0].xpos     = rx + otherLeft;

            sensors[0].ypos = ry + otherTop + 0x20000;
            sensors[1].ypos = ry + otherBottom - 0x20000;
            for (int32 i = 0; i < 2; ++i) {
                if (thisRight > sensors[0].xpos && thisRight < otherEntity->xpos - otherEntity->xvel && thisTop < sensors[1].ypos
                    && thisBottom > sensors[0].ypos) {
                    sensors[i].collided = true;
                }
            }

            if (sensors[0].collided || sensors[1].collided) {
                otherEntity->xpos = thisRight - otherLeft;
                if (otherEntity->xvel < 0) {
                    if (otherEntity->direction == FLIP_X)
                        otherEntity->pushing = 2;

                    if (otherEntity->xvel < -0x10000)
                        otherEntity->xpos += 0x8000;

                    otherEntity->xvel = 0;
                    if (otherEntity->collisionMode || !otherEntity->right)
                        otherEntity->speed = 0;
                    else
                        otherEntity->speed = 0x8000;
                }
                scriptEng.checkResult = 3;
            }
            else {
                sensors[0].collided = false;
                sensors[1].collided = false;
                sensors[2].collided = false;
                sensors[3].collided = false;
                sensors[4].collided = false;
                sensors[0].xpos     = rx + otherLeft + 0x20000;
                sensors[1].xpos     = rx;
                sensors[2].xpos     = rx + otherRight - 0x20000;
                sensors[3].xpos     = (sensors[0].xpos + rx) >> 1;
                sensors[4].xpos     = (sensors[2].xpos + rx) >> 1;

                sensors[0].ypos = ry + otherBottom;
                if (otherEntity->yvel >= 0) {
                    for (int32 i = 0; i < 5; ++i) {
                        if (thisLeft < sensors[i].xpos && thisRight > sensors[i].xpos && thisTop <= sensors[0].ypos
                            && thisTop > otherEntity->ypos - otherEntity->yvel) {
                            sensors[i].collided          = true;
                            otherEntity->floorSensors[i] = true;
                        }
                    }
                }
                if (sensors[2].collided || sensors[1].collided || sensors[0].collided) {
                    if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                        otherEntity->xvel  = 0;
                        otherEntity->speed = 0;
                    }
                    otherEntity->ypos        = thisTop - otherBottom;
                    otherEntity->gravity     = 0;
                    otherEntity->yvel        = 0;
                    otherEntity->angle       = 0;
                    otherEntity->rotation    = 0;
                    otherEntity->controlLock = 0;
                    scriptEng.checkResult    = 1;
                }
                else {
                    sensors[0].collided = false;
                    sensors[1].collided = false;
                    sensors[0].xpos     = rx + otherLeft + 0x20000;
                    sensors[1].xpos     = rx + otherRight - 0x20000;
                    sensors[0].ypos     = ry + otherTop;

                    for (int32 i = 0; i < 2; ++i) {
                        if (thisLeft < sensors[1].xpos && thisRight > sensors[0].xpos && thisBottom > sensors[0].ypos
                            && thisBottom < otherEntity->ypos - otherEntity->yvel) {
                            sensors[i].collided = true;
                        }
                    }

                    if (sensors[1].collided || sensors[0].collided) {
                        if (otherEntity->gravity == 1)
                            otherEntity->ypos = thisBottom - otherTop;

                        if (otherEntity->yvel <= 0)
                            otherEntity->yvel = 0;
                        scriptEng.checkResult = 4;
                    }
                }
            }
        }
    }

#if !RETRO_USE_ORIGINAL_CODE
    if (showHitboxes) {
        if (thisHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[thisHitboxID].collision |= 1 << (scriptEng.checkResult - 1);
        if (otherHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[otherHitboxID].collision |= 1 << (4 - scriptEng.checkResult);
    }
#endif
}
void RSDK::Legacy::v4::BoxCollision2(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity,
                                     int32 otherLeft, int32 otherTop, int32 otherRight, int32 otherBottom)
{
    Hitbox *thisHitbox  = GetHitbox(thisEntity);
    Hitbox *otherHitbox = GetHitbox(otherEntity);

    if (thisLeft == C_BOX)
        thisLeft = thisHitbox->left[0];

    if (thisTop == C_BOX)
        thisTop = thisHitbox->top[0];

    if (thisRight == C_BOX)
        thisRight = thisHitbox->right[0];

    if (thisBottom == C_BOX)
        thisBottom = thisHitbox->bottom[0];

    if (otherLeft == C_BOX)
        otherLeft = otherHitbox->left[0];

    if (otherTop == C_BOX)
        otherTop = otherHitbox->top[0];

    if (otherRight == C_BOX)
        otherRight = otherHitbox->right[0];

    if (otherBottom == C_BOX)
        otherBottom = otherHitbox->bottom[0];

#if !RETRO_USE_ORIGINAL_CODE
    int32 thisHitboxID  = 0;
    int32 otherHitboxID = 0;
    if (showHitboxes) {
        thisHitboxID  = AddDebugHitbox(H_TYPE_BOX, thisEntity, thisLeft, thisTop, thisRight, thisBottom);
        otherHitboxID = AddDebugHitbox(H_TYPE_BOX, otherEntity, otherLeft, otherTop, otherRight, otherBottom);
    }
#endif

    thisLeft += thisEntity->xpos >> 16;
    thisTop += thisEntity->ypos >> 16;
    thisRight += thisEntity->xpos >> 16;
    thisBottom += thisEntity->ypos >> 16;

    thisLeft <<= 16;
    thisTop <<= 16;
    thisRight <<= 16;
    thisBottom <<= 16;

    otherLeft <<= 16;
    otherTop <<= 16;
    otherRight <<= 16;
    otherBottom <<= 16;

    scriptEng.checkResult = 0;

    int32 rx = otherEntity->xpos >> 16 << 16;
    int32 ry = otherEntity->ypos >> 16 << 16;

    int32 xDif = thisLeft - rx;
    if (thisEntity->xpos <= rx)
        xDif = rx - thisRight;
    int32 yDif = thisTop - ry;
    if (thisEntity->ypos <= ry)
        yDif = ry - thisBottom;

    if (xDif <= yDif) {
        sensors[0].collided = false;
        sensors[1].collided = false;
        sensors[2].collided = false;
        sensors[0].xpos     = rx + otherLeft + 0x20000;
        sensors[1].xpos     = rx;
        sensors[2].xpos     = rx + otherRight - 0x20000;

        sensors[0].ypos = ry + otherBottom;

        if (otherEntity->yvel >= 0) {
            // this should prolly be using all 5 sensors, but this was unused in S2 so it was prolly forgotten about
            for (int32 i = 0; i < 3; ++i) {
                if (thisLeft < sensors[i].xpos && thisRight > sensors[i].xpos && thisTop <= sensors[0].ypos && thisEntity->ypos > sensors[0].ypos) {
                    sensors[i].collided          = true;
                    otherEntity->floorSensors[i] = true;
                }
            }
        }

        if (sensors[0].collided || sensors[1].collided || sensors[2].collided) {
            if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                otherEntity->xvel  = 0;
                otherEntity->speed = 0;
            }
            otherEntity->ypos        = thisTop - otherBottom;
            otherEntity->gravity     = 0;
            otherEntity->yvel        = 0;
            otherEntity->angle       = 0;
            otherEntity->rotation    = 0;
            otherEntity->controlLock = 0;
            scriptEng.checkResult    = 1;
        }
        else {
            sensors[0].collided = false;
            sensors[1].collided = false;
            sensors[0].xpos     = rx + otherLeft + 0x20000;
            sensors[1].xpos     = rx + otherRight - 0x20000;

            sensors[0].ypos = ry + otherTop;

            for (int32 i = 0; i < 2; ++i) {
                if (thisLeft < sensors[1].xpos && thisRight > sensors[0].xpos && thisBottom > sensors[0].ypos && thisEntity->ypos < sensors[0].ypos) {
                    sensors[i].collided = true;
                }
            }

            if (sensors[1].collided || sensors[0].collided) {
                if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                    otherEntity->xvel  = 0;
                    otherEntity->speed = 0;
                }

                otherEntity->ypos = thisBottom - otherTop;
                if (otherEntity->yvel < 0)
                    otherEntity->yvel = 0;
                scriptEng.checkResult = 4;
            }
            else {
                sensors[0].collided = false;
                sensors[1].collided = false;
                sensors[0].xpos     = rx + otherRight;

                sensors[0].ypos = ry + otherTop + 0x20000;
                sensors[1].ypos = ry + otherBottom - 0x20000;
                for (int32 i = 0; i < 2; ++i) {
                    if (thisLeft <= sensors[0].xpos && thisEntity->xpos > sensors[0].xpos && thisTop < sensors[1].ypos
                        && thisBottom > sensors[0].ypos) {
                        sensors[i].collided = true;
                    }
                }

                if (sensors[1].collided || sensors[0].collided) {
                    otherEntity->xpos = thisLeft - otherRight;
                    if (otherEntity->xvel > 0) {
                        if (!otherEntity->direction)
                            otherEntity->pushing = 2;

                        otherEntity->xvel  = 0;
                        otherEntity->speed = 0;
                    }
                    scriptEng.checkResult = 2;
                }
                else {
                    sensors[0].collided = false;
                    sensors[1].collided = false;
                    sensors[0].xpos     = rx + otherLeft;

                    sensors[0].ypos = ry + otherTop + 0x20000;
                    sensors[1].ypos = ry + otherBottom - 0x20000;
                    for (int32 i = 0; i < 2; ++i) {
                        if (thisRight > sensors[0].xpos && thisEntity->xpos < sensors[0].xpos && thisTop < sensors[1].ypos
                            && thisBottom > sensors[0].ypos) {
                            sensors[i].collided = true;
                        }
                    }

                    if (sensors[1].collided || sensors[0].collided) {
                        otherEntity->xpos = thisRight - otherLeft;
                        if (otherEntity->xvel < 0) {
                            if (otherEntity->direction == FLIP_X)
                                otherEntity->pushing = 2;

                            if (otherEntity->xvel < -0x10000)
                                otherEntity->xpos += 0x8000;

                            otherEntity->xvel  = 0;
                            otherEntity->speed = 0;
                        }
                        scriptEng.checkResult = 3;
                    }
                }
            }
        }
    }
    else {
        sensors[0].collided = false;
        sensors[1].collided = false;
        sensors[0].xpos     = rx + otherRight;

        sensors[0].ypos = ry + otherTop + 0x20000;
        sensors[1].ypos = ry + otherBottom - 0x20000;
        for (int32 i = 0; i < 2; ++i) {
            if (thisLeft <= sensors[0].xpos && thisEntity->xpos > sensors[0].xpos && thisTop < sensors[1].ypos && thisBottom > sensors[0].ypos) {
                sensors[i].collided = true;
            }
        }
        if (sensors[1].collided || sensors[0].collided) {
            otherEntity->xpos = thisLeft - otherRight;
            if (otherEntity->xvel > 0) {
                if (!otherEntity->direction)
                    otherEntity->pushing = 2;

                otherEntity->xvel  = 0;
                otherEntity->speed = 0;
            }
            scriptEng.checkResult = 2;
        }
        else {
            sensors[0].collided = false;
            sensors[1].collided = false;
            sensors[0].xpos     = rx + otherLeft;

            sensors[0].ypos = ry + otherTop + 0x20000;
            sensors[1].ypos = ry + otherBottom - 0x20000;
            for (int32 i = 0; i < 2; ++i) {
                if (thisRight > sensors[0].xpos && thisEntity->xpos < sensors[0].xpos && thisTop < sensors[1].ypos && thisBottom > sensors[0].ypos) {
                    sensors[i].collided = true;
                }
            }

            if (sensors[0].collided || sensors[1].collided) {
                otherEntity->xpos = thisRight - otherLeft;
                if (otherEntity->xvel < 0) {
                    if (otherEntity->direction == FLIP_X)
                        otherEntity->pushing = 2;

                    if (otherEntity->xvel < -0x10000)
                        otherEntity->xpos += 0x8000;

                    otherEntity->xvel  = 0;
                    otherEntity->speed = 0;
                }
                scriptEng.checkResult = 3;
            }
            else {
                sensors[0].collided = false;
                sensors[1].collided = false;
                sensors[2].collided = false;
                sensors[0].xpos     = rx + otherLeft + 0x20000;
                sensors[1].xpos     = rx;
                sensors[2].xpos     = rx + otherRight - 0x20000;

                sensors[0].ypos = ry + otherBottom;
                if (otherEntity->yvel >= 0) {
                    for (int32 i = 0; i < 3; ++i) {
                        if (thisLeft < sensors[i].xpos && thisRight > sensors[i].xpos && thisTop <= sensors[0].ypos
                            && thisEntity->ypos > sensors[0].ypos) {
                            sensors[i].collided          = true;
                            otherEntity->floorSensors[i] = true;
                        }
                    }
                }

                if (sensors[0].collided || sensors[1].collided || sensors[2].collided) {
                    if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                        otherEntity->xvel  = 0;
                        otherEntity->speed = 0;
                    }
                    otherEntity->ypos        = thisTop - otherBottom;
                    otherEntity->gravity     = 0;
                    otherEntity->yvel        = 0;
                    otherEntity->angle       = 0;
                    otherEntity->rotation    = 0;
                    otherEntity->controlLock = 0;
                    scriptEng.checkResult    = 1;
                }
                else {
                    sensors[0].collided = false;
                    sensors[1].collided = false;
                    sensors[0].xpos     = rx + otherLeft + 0x20000;
                    sensors[1].xpos     = rx + otherRight - 0x20000;

                    sensors[0].ypos = ry + otherTop;

                    for (int32 i = 0; i < 2; ++i) {
                        if (thisLeft < sensors[1].xpos && thisRight > sensors[0].xpos && thisBottom > sensors[0].ypos
                            && thisEntity->ypos < sensors[0].ypos) {
                            sensors[i].collided = true;
                        }
                    }

                    if (sensors[1].collided || sensors[0].collided) {
                        if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
                            otherEntity->xvel  = 0;
                            otherEntity->speed = 0;
                        }

                        otherEntity->ypos = thisBottom - otherTop;

                        if (otherEntity->yvel < 0)
                            otherEntity->yvel = 0;
                        scriptEng.checkResult = 4;
                    }
                }
            }
        }
    }

#if !RETRO_USE_ORIGINAL_CODE
    if (showHitboxes) {
        if (thisHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[thisHitboxID].collision |= 1 << (scriptEng.checkResult - 1);
        if (otherHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[otherHitboxID].collision |= 1 << (4 - scriptEng.checkResult);
    }
#endif
}
void RSDK::Legacy::v4::PlatformCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity,
                                         int32 otherLeft, int32 otherTop, int32 otherRight, int32 otherBottom)
{
    scriptEng.checkResult = false;

    Hitbox *thisHitbox  = GetHitbox(thisEntity);
    Hitbox *otherHitbox = GetHitbox(otherEntity);

    if (thisLeft == C_BOX)
        thisLeft = thisHitbox->left[0];

    if (thisTop == C_BOX)
        thisTop = thisHitbox->top[0];

    if (thisRight == C_BOX)
        thisRight = thisHitbox->right[0];

    if (thisBottom == C_BOX)
        thisBottom = thisHitbox->bottom[0];

    if (otherLeft == C_BOX)
        otherLeft = otherHitbox->left[0];

    if (otherTop == C_BOX)
        otherTop = otherHitbox->top[0];

    if (otherRight == C_BOX)
        otherRight = otherHitbox->right[0];

    if (otherBottom == C_BOX)
        otherBottom = otherHitbox->bottom[0];

#if !RETRO_USE_ORIGINAL_CODE
    int32 thisHitboxID  = 0;
    int32 otherHitboxID = 0;
    if (showHitboxes) {
        thisHitboxID  = AddDebugHitbox(H_TYPE_PLAT, thisEntity, thisLeft, thisTop, thisRight, thisBottom);
        otherHitboxID = AddDebugHitbox(H_TYPE_PLAT, otherEntity, otherLeft, otherTop, otherRight, otherBottom);
    }
#endif

    thisLeft += thisEntity->xpos >> 16;
    thisTop += thisEntity->ypos >> 16;
    thisRight += thisEntity->xpos >> 16;
    thisBottom += thisEntity->ypos >> 16;

    thisLeft <<= 16;
    thisTop <<= 16;
    thisRight <<= 16;
    thisBottom <<= 16;

    sensors[0].collided = false;
    sensors[1].collided = false;
    sensors[2].collided = false;

    int32 rx = otherEntity->xpos >> 16 << 16;
    int32 ry = otherEntity->ypos >> 16 << 16;

    sensors[0].xpos = rx + (otherLeft << 16);
    sensors[1].xpos = rx;
    sensors[2].xpos = rx + (otherRight << 16);
    sensors[3].xpos = (rx + sensors[0].xpos) >> 1;
    sensors[4].xpos = (sensors[2].xpos + rx) >> 1;

    sensors[0].ypos = (otherBottom << 16) + ry;

    for (int32 i = 0; i < 5; ++i) {
        if (thisLeft < sensors[i].xpos && thisRight > sensors[i].xpos && thisTop - 1 <= sensors[0].ypos && thisBottom > sensors[0].ypos
            && otherEntity->yvel >= 0) {
            sensors[i].collided          = true;
            otherEntity->floorSensors[i] = true;
        }
    }

    if (sensors[0].collided || sensors[1].collided || sensors[2].collided) {
        if (!otherEntity->gravity && (otherEntity->collisionMode == CMODE_RWALL || otherEntity->collisionMode == CMODE_LWALL)) {
            otherEntity->xvel  = 0;
            otherEntity->speed = 0;
        }
        otherEntity->ypos        = thisTop - (otherBottom << 16);
        otherEntity->gravity     = 0;
        otherEntity->yvel        = 0;
        otherEntity->angle       = 0;
        otherEntity->rotation    = 0;
        otherEntity->controlLock = 0;
        scriptEng.checkResult    = true;
    }

#if !RETRO_USE_ORIGINAL_CODE
    if (showHitboxes) {
        if (thisHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[thisHitboxID].collision |= 1 << 0;
        if (otherHitboxID >= 0 && scriptEng.checkResult)
            debugHitboxList[otherHitboxID].collision |= 1 << 3;
    }
#endif
}
