#include "AffinitySystem.h"

// ─── レベル定義 ────────────────────────────────────────────────────
//  Neutral  :    0 - 249  (interval 30-60s, motions: shy)
//  Friend   :  250 - 499  (interval 20-40s, motions: shy, happy)
//  Close    :  500 - 749  (interval 10-25s, motions: happy, excited)
//  Love     :  750 - 1000 (interval  5-15s, motions: excited, love)
// ──────────────────────────────────────────────────────────────────

struct LevelInfo
{
    int minPoints;
    int maxPoints;
    int minIntervalMs;
    int maxIntervalMs;
    const char* motions[4]; // null-terminated list
};

static const LevelInfo kLevelTable[] =
{
    {   0, 249, 30000, 60000, { AffinitySystem::MOTION_SHY,     nullptr,                         nullptr,                          nullptr } },
    { 250, 499, 20000, 40000, { AffinitySystem::MOTION_SHY,     AffinitySystem::MOTION_HAPPY,    nullptr,                          nullptr } },
    { 500, 749, 10000, 25000, { AffinitySystem::MOTION_HAPPY,   AffinitySystem::MOTION_EXCITED,  nullptr,                          nullptr } },
    { 750,1000,  5000, 15000, { AffinitySystem::MOTION_EXCITED, AffinitySystem::MOTION_LOVE,     AffinitySystem::MOTION_LOVE,      nullptr } },
};

// ──────────────────────────────────────────────────────────────────

AffinitySystem::AffinitySystem()
{
    scheduleNextEvent();
}

AffinitySystem::~AffinitySystem()
{
    stopTimer();
}

int AffinitySystem::levelThreshold(Level l)
{
    return kLevelTable[static_cast<int>(l)].minPoints;
}

int AffinitySystem::levelMax(Level l)
{
    return kLevelTable[static_cast<int>(l)].maxPoints;
}

juce::String AffinitySystem::levelName(Level l)
{
    switch (l)
    {
        case Level::Neutral: return "Neutral";
        case Level::Friend:  return "Friend";
        case Level::Close:   return "Close";
        case Level::Love:    return "Love";
    }
    return "Neutral";
}

float AffinitySystem::getProgress() const
{
    const auto& info = kLevelTable[static_cast<int>(level_)];
    int range = info.maxPoints - info.minPoints;
    if (range <= 0) return 1.0f;
    return juce::jlimit(0.f, 1.f,
           (float)(points_ - info.minPoints) / (float)range);
}

AffinitySystem::Level AffinitySystem::calcLevel() const
{
    if (points_ >= 750) return Level::Love;
    if (points_ >= 500) return Level::Close;
    if (points_ >= 250) return Level::Friend;
    return Level::Neutral;
}

void AffinitySystem::addAffinity(int pts)
{
    points_ = juce::jlimit(0, 1000, points_ + pts);
    auto newLevel = calcLevel();
    if (newLevel != level_)
    {
        level_ = newLevel;
        if (levelCallback_)
            levelCallback_(level_);
        // レベルアップ時は即座に対応モーションを発火
        if (motionCallback_)
            motionCallback_(pickRandomMotion());
        scheduleNextEvent();
    }
}

void AffinitySystem::timerCallback()
{
    if (motionCallback_)
        motionCallback_(pickRandomMotion());
    scheduleNextEvent();
}

void AffinitySystem::scheduleNextEvent()
{
    const auto& info = kLevelTable[static_cast<int>(level_)];
    int interval = info.minIntervalMs
                 + rng_.nextInt(info.maxIntervalMs - info.minIntervalMs + 1);
    startTimer(interval);
}

juce::String AffinitySystem::pickRandomMotion() const
{
    const auto& info = kLevelTable[static_cast<int>(level_)];
    int count = 0;
    while (count < 4 && info.motions[count] != nullptr)
        ++count;
    if (count == 0) return MOTION_SHY;
    return juce::String(info.motions[rng_.nextInt(count)]);
}

void AffinitySystem::saveToValueTree(juce::ValueTree& tree) const
{
    tree.setProperty("affinityPoints", points_, nullptr);
}

void AffinitySystem::loadFromValueTree(const juce::ValueTree& tree)
{
    points_ = juce::jlimit(0, 1000, (int)tree.getProperty("affinityPoints", 0));
    level_  = calcLevel();
    scheduleNextEvent();
}
