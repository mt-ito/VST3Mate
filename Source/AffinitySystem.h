#pragma once
#include <JuceHeader.h>

/**
 * キャラクター好感度システム
 *
 * 好感度ポイント (0-1000) を管理し、4段階のレベルに応じて
 * ランダムタイミングでモーション (アニメーション) イベントを発火する。
 *
 * Levels:
 *   Neutral  (0-249)  : 波モーションが 30-60 秒おきに発火
 *   Friend   (250-499): happy/wave が 20-40 秒おきに発火
 *   Close    (500-749): happy/excited が 10-25 秒おきに発火
 *   Love     (750+)   : excited/love が  5-15 秒おきに発火
 */
class AffinitySystem : private juce::Timer
{
public:
    enum class Level { Neutral = 0, Friend = 1, Close = 2, Love = 3 };

    // 発火するモーション名 → CharacterComponent の AnimState と対応
    static constexpr const char* MOTION_SHY     = "shy";
    static constexpr const char* MOTION_HAPPY   = "happy";
    static constexpr const char* MOTION_EXCITED = "excited";
    static constexpr const char* MOTION_LOVE    = "love";

    // motionName を受け取るコールバック (message thread で呼ばれる)
    using MotionCallback = std::function<void(const juce::String& motionName)>;
    using LevelCallback  = std::function<void(Level newLevel)>;

    AffinitySystem();
    ~AffinitySystem() override;

    // ノートを生成したときなどに呼ぶ
    void addAffinity(int points);

    // 現在のポイントとレベル
    int   getPoints() const { return points_; }
    Level getLevel()  const { return level_; }
    float getProgress() const; // 現在レベル内の進捗 0.0-1.0

    // コールバック設定
    void setMotionCallback(MotionCallback cb) { motionCallback_ = std::move(cb); }
    void setLevelCallback(LevelCallback cb)   { levelCallback_  = std::move(cb); }

    // 状態の永続化
    void saveToValueTree(juce::ValueTree& tree) const;
    void loadFromValueTree(const juce::ValueTree& tree);

    static juce::String levelName(Level l);
    static int levelThreshold(Level l); // そのレベルの開始ポイント
    static int levelMax(Level l);       // そのレベルの終了ポイント

private:
    void timerCallback() override;
    void scheduleNextEvent();
    juce::String pickRandomMotion() const;
    Level calcLevel() const;

    int   points_ = 0;
    Level level_  = Level::Neutral;

    MotionCallback motionCallback_;
    LevelCallback  levelCallback_;

    mutable juce::Random rng_;
};
