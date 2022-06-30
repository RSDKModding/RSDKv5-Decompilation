

RSDK::Legacy::AnimationFile RSDK::Legacy::animationFileList[LEGACY_ANIFILE_COUNT];
int32 RSDK::Legacy::animationFileCount = 0;

RSDK::Legacy::SpriteFrame RSDK::Legacy::scriptFrames[LEGACY_SPRITEFRAME_COUNT];
int32 RSDK::Legacy::scriptFrameCount = 0;

RSDK::Legacy::SpriteFrame RSDK::Legacy::animFrames[LEGACY_SPRITEFRAME_COUNT];
int32 RSDK::Legacy::animFrameCount = 0;
RSDK::Legacy::SpriteAnimation RSDK::Legacy::animationList[LEGACY_ANIMATION_COUNT];
int32 RSDK::Legacy::animationCount = 0;
RSDK::Legacy::Hitbox RSDK::Legacy::hitboxList[LEGACY_HITBOX_COUNT];
int32 RSDK::Legacy::hitboxCount = 0;

void RSDK::Legacy::LoadAnimationFile(char *filePath)
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, filePath, FMODE_RB)) {
        byte sheetIDs[24];
        sheetIDs[0] = 0;

        byte sheetCount = ReadInt8(&info);

        for (int32 s = 0; s < sheetCount; ++s) {
            char sheetPath[0x21];
            ReadString(&info, sheetPath);
            sheetIDs[s] = AddGraphicsFile(sheetPath);
        }

        AnimationFile *animFile = &animationFileList[animationFileCount];
        animFile->animCount     = ReadInt8(&info);
        animFile->aniListOffset = animationCount;

        for (int32 a = 0; a < animFile->animCount; ++a) {
            SpriteAnimation *anim = &animationList[animationCount++];
            anim->frameListOffset = animFrameCount;

            ReadString(&info, anim->name);
            anim->frameCount    = ReadInt8(&info);
            anim->speed         = ReadInt8(&info);
            anim->loopPoint     = ReadInt8(&info);
            anim->rotationStyle = ReadInt8(&info);

            for (int32 f = 0; f < anim->frameCount; ++f) {
                SpriteFrame *frame = &animFrames[animFrameCount++];
                frame->sheetID     = sheetIDs[ReadInt8(&info)];
                frame->hitboxID    = ReadInt8(&info);
                frame->sprX        = ReadInt8(&info);
                frame->sprY        = ReadInt8(&info);
                frame->width       = ReadInt8(&info);
                frame->height      = ReadInt8(&info);
                frame->pivotX      = (int8)ReadInt8(&info);
                frame->pivotY      = (int8)ReadInt8(&info);
            }

            // 90 Degree (Extra rotation Frames) rotation
            if (anim->rotationStyle == ROTSTYLE_STATICFRAMES)
                anim->frameCount >>= 1;
        }

        animFile->hitboxListOffset = hitboxCount;
        int32 hbCount              = ReadInt8(&info);
        for (int32 h = 0; h < hbCount; ++h) {
            Hitbox *hitbox = &hitboxList[hitboxCount++];
            for (int32 d = 0; d < LEGACY_HITBOX_DIR_COUNT; ++d) {
                hitbox->left[d]   = ReadInt8(&info);
                hitbox->top[d]    = ReadInt8(&info);
                hitbox->right[d]  = ReadInt8(&info);
                hitbox->bottom[d] = ReadInt8(&info);
            }
        }

        CloseFile(&info);
    }
}
void RSDK::Legacy::ClearAnimationData()
{
    for (int32 f = 0; f < LEGACY_SPRITEFRAME_COUNT; ++f) MEM_ZERO(scriptFrames[f]);
    for (int32 f = 0; f < LEGACY_SPRITEFRAME_COUNT; ++f) MEM_ZERO(animFrames[f]);
    for (int32 h = 0; h < LEGACY_HITBOX_COUNT; ++h) MEM_ZERO(hitboxList[h]);
    for (int32 a = 0; a < LEGACY_ANIMATION_COUNT; ++a) MEM_ZERO(animationList[a]);
    for (int32 a = 0; a < LEGACY_ANIFILE_COUNT; ++a) MEM_ZERO(animationFileList[a]);

    scriptFrameCount   = 0;
    animFrameCount     = 0;
    animationCount     = 0;
    animationFileCount = 0;
    hitboxCount        = 0;
}

RSDK::Legacy::AnimationFile *RSDK::Legacy::AddAnimationFile(char *filePath)
{
    char path[0x80];
    StrCopy(path, "Data/Animations/");
    StrAdd(path, filePath);

    for (int32 a = 0; a < LEGACY_ANIFILE_COUNT; ++a) {
        if (StrLength(animationFileList[a].fileName) <= 0) {
            StrCopy(animationFileList[a].fileName, filePath);
            LoadAnimationFile(path);
            ++animationFileCount;
            return &animationFileList[a];
        }

        if (StrComp(animationFileList[a].fileName, filePath))
            return &animationFileList[a];
    }

    return NULL;
}

void RSDK::Legacy::v3::ProcessObjectAnimation(void *objScr, void *ent)
{
    Legacy::v3::ObjectScript *objectScript = (Legacy::v3::ObjectScript *)objScr;
    Legacy::v3::Entity *entity             = (Legacy::v3::Entity *)ent;
    Legacy::SpriteAnimation *sprAnim   = &Legacy::animationList[objectScript->animFile->aniListOffset + entity->animation];

    if (entity->animationSpeed <= 0) {
        entity->animationTimer += sprAnim->speed;
    }
    else {
        if (entity->animationSpeed > 0xF0)
            entity->animationSpeed = 0xF0;
        entity->animationTimer += entity->animationSpeed;
    }

    if (entity->animation != entity->prevAnimation) {
        entity->prevAnimation  = entity->animation;
        entity->frame          = 0;
        entity->animationTimer = 0;
        entity->animationSpeed = 0;
    }

    if (entity->animationTimer >= 0xF0) {
        entity->animationTimer -= 0xF0;
        ++entity->frame;
    }

    if (entity->frame >= sprAnim->frameCount)
        entity->frame = sprAnim->loopPoint;
}

void RSDK::Legacy::v4::ProcessObjectAnimation(void *objScr, void *ent)
{
    Legacy::v4::ObjectScript *objectScript = (Legacy::v4::ObjectScript *)objScr;
    Legacy::v4::Entity *entity             = (Legacy::v4::Entity *)ent;
    Legacy::SpriteAnimation *sprAnim       = &Legacy::animationList[objectScript->animFile->aniListOffset + entity->animation];

    if (entity->animationSpeed <= 0) {
        entity->animationTimer += sprAnim->speed;
    }
    else {
        if (entity->animationSpeed > 0xF0)
            entity->animationSpeed = 0xF0;
        entity->animationTimer += entity->animationSpeed;
    }

    if (entity->animation != entity->prevAnimation) {
        entity->prevAnimation  = entity->animation;
        entity->frame          = 0;
        entity->animationTimer = 0;
        entity->animationSpeed = 0;
    }

    if (entity->animationTimer >= 0xF0) {
        entity->animationTimer -= 0xF0;
        ++entity->frame;
    }

    if (entity->frame >= sprAnim->frameCount)
        entity->frame = sprAnim->loopPoint;
}