#if RETRO_REV02

struct SteamAchievements : UserAchievements {
    void TryUnlockAchievement(AchievementID *id)
    {
        if (id) {
            // try unlock a steam achievement
        }
    }
};

#endif
