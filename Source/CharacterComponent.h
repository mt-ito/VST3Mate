#pragma once
#include <JuceHeader.h>

enum class AnimState
{
    Idle,
    Clicked,
    Triggered,
    Happy,    // 好感度: Friend以上でランダム発火
    Excited,  // 好感度: Close以上でランダム発火
    Love,     // 好感度: Love でランダム発火
    Shy,      // 好感度: Neutral でランダム発火
};

class CharacterComponent : public juce::Component, private juce::Timer
{
public:
    using ClickCallback = std::function<void()>;

    CharacterComponent();
    ~CharacterComponent() override;

    // Load sprite frames from a directory (sorted by filename)
    void loadFrames(const juce::File& directory);

    // Load idle and click animation directories
    void loadAnimations(const juce::File& idleDir, const juce::File& clickedDir);

    // Set all frame sets directly from image arrays (e.g., from BinaryData)
    void setFrames(std::vector<juce::Image> idleFrames,
                   std::vector<juce::Image> clickedFrames);

    // Set individual affinity-state frames
    void setAffinityFrames(std::vector<juce::Image> happyFrames,
                           std::vector<juce::Image> excitedFrames,
                           std::vector<juce::Image> loveFrames,
                           std::vector<juce::Image> shyFrames);

    void setAnimState(AnimState state);
    AnimState getAnimState() const { return currentState_; }

    void setFps(int fps);
    void setClickCallback(ClickCallback cb) { clickCallback_ = std::move(cb); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void advanceFrame();
    const std::vector<juce::Image>& currentFrames() const;

    std::vector<juce::Image> idleFrames_;
    std::vector<juce::Image> clickedFrames_;
    std::vector<juce::Image> happyFrames_;
    std::vector<juce::Image> excitedFrames_;
    std::vector<juce::Image> loveFrames_;
    std::vector<juce::Image> shyFrames_;

    AnimState currentState_ = AnimState::Idle;
    int       currentFrame_ = 0;
    int       fps_          = 10;

    bool revertToIdleAfterCycle_ = false;

    ClickCallback clickCallback_;

    void paintFallbackCharacter(juce::Graphics& g);
};
