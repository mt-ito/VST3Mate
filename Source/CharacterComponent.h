#pragma once
#include <JuceHeader.h>

enum class AnimState { Idle, Clicked, Triggered };

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

    // Set frames directly from image arrays (e.g., from BinaryData)
    void setFrames(std::vector<juce::Image> idleFrames,
                   std::vector<juce::Image> clickedFrames);

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

    AnimState currentState_ = AnimState::Idle;
    int       currentFrame_ = 0;
    int       fps_          = 10;

    // After a click/trigger animation finishes, revert to idle
    bool revertToIdleAfterCycle_ = false;

    ClickCallback clickCallback_;

    // Fallback drawn character when no frames loaded
    void paintFallbackCharacter(juce::Graphics& g);
};
