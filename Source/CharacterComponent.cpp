#include "CharacterComponent.h"

CharacterComponent::CharacterComponent()
{
    setSize(200, 200);
    startTimer(1000 / fps_);
}

CharacterComponent::~CharacterComponent()
{
    stopTimer();
}

static std::vector<juce::Image> loadFramesFromDir(const juce::File& dir)
{
    std::vector<juce::Image> frames;
    if (!dir.isDirectory()) return frames;

    juce::Array<juce::File> files;
    dir.findChildFiles(files, juce::File::findFiles, false, "*.png");
    files.sort();

    for (auto& f : files)
    {
        auto img = juce::ImageFileFormat::loadFrom(f);
        if (img.isValid())
            frames.push_back(img);
    }
    return frames;
}

void CharacterComponent::loadFrames(const juce::File& directory)
{
    idleFrames_ = loadFramesFromDir(directory);
    currentFrame_ = 0;
    repaint();
}

void CharacterComponent::loadAnimations(const juce::File& idleDir, const juce::File& clickedDir)
{
    idleFrames_    = loadFramesFromDir(idleDir);
    clickedFrames_ = loadFramesFromDir(clickedDir);
    currentFrame_  = 0;
    repaint();
}

void CharacterComponent::setFrames(std::vector<juce::Image> idleFrames,
                                    std::vector<juce::Image> clickedFrames)
{
    idleFrames_    = std::move(idleFrames);
    clickedFrames_ = std::move(clickedFrames);
    currentFrame_  = 0;
    repaint();
}

void CharacterComponent::setAffinityFrames(std::vector<juce::Image> happyFrames,
                                            std::vector<juce::Image> excitedFrames,
                                            std::vector<juce::Image> loveFrames,
                                            std::vector<juce::Image> shyFrames)
{
    happyFrames_   = std::move(happyFrames);
    excitedFrames_ = std::move(excitedFrames);
    loveFrames_    = std::move(loveFrames);
    shyFrames_     = std::move(shyFrames);
}

void CharacterComponent::setAnimState(AnimState state)
{
    if (currentState_ == state) return;
    currentState_ = state;
    currentFrame_ = 0;

    // 一発モーション (再生後 Idle に戻る)
    revertToIdleAfterCycle_ = (state != AnimState::Idle);
    repaint();
}

void CharacterComponent::setFps(int fps)
{
    fps_ = juce::jmax(1, fps);
    startTimer(1000 / fps_);
}

void CharacterComponent::timerCallback()
{
    advanceFrame();
}

void CharacterComponent::advanceFrame()
{
    const auto& frames = currentFrames();
    if (frames.empty())
    {
        repaint();
        return;
    }

    currentFrame_ = (currentFrame_ + 1) % (int)frames.size();

    if (revertToIdleAfterCycle_ && currentFrame_ == 0)
    {
        currentState_           = AnimState::Idle;
        revertToIdleAfterCycle_ = false;
    }
    repaint();
}

const std::vector<juce::Image>& CharacterComponent::currentFrames() const
{
    auto fallbackToIdle = [this]() -> const std::vector<juce::Image>& { return idleFrames_; };

    switch (currentState_)
    {
        case AnimState::Clicked:
        case AnimState::Triggered:
            return clickedFrames_.empty()  ? fallbackToIdle() : clickedFrames_;
        case AnimState::Happy:
            return happyFrames_.empty()    ? fallbackToIdle() : happyFrames_;
        case AnimState::Excited:
            return excitedFrames_.empty()  ? fallbackToIdle() : excitedFrames_;
        case AnimState::Love:
            return loveFrames_.empty()     ? fallbackToIdle() : loveFrames_;
        case AnimState::Shy:
            return shyFrames_.empty()      ? fallbackToIdle() : shyFrames_;
        default:
            return idleFrames_;
    }
}

void CharacterComponent::paint(juce::Graphics& g)
{
    const auto& frames = currentFrames();
    if (!frames.empty() && currentFrame_ < (int)frames.size())
    {
        g.drawImageWithin(frames[currentFrame_],
                          0, 0, getWidth(), getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        paintFallbackCharacter(g);
    }
}

void CharacterComponent::mouseDown(const juce::MouseEvent& /*e*/)
{
    setAnimState(AnimState::Clicked);
    if (clickCallback_) clickCallback_();
}

void CharacterComponent::paintFallbackCharacter(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10.f);

    g.setColour(juce::Colours::lightblue);
    g.fillEllipse(bounds.withSizeKeepingCentre(bounds.getWidth() * 0.6f, bounds.getHeight() * 0.7f));

    float headR = bounds.getWidth() * 0.28f;
    float headX = bounds.getCentreX() - headR;
    float headY = bounds.getY() + 4.f;
    g.setColour(juce::Colours::peachpuff);
    g.fillEllipse(headX, headY, headR * 2.f, headR * 2.f);

    float eyeY = headY + headR * 0.6f;
    g.setColour(juce::Colours::darkblue);

    bool eyeClosed = (currentState_ == AnimState::Clicked && currentFrame_ < 2)
                  || (currentState_ == AnimState::Shy);
    if (eyeClosed)
    {
        g.drawLine(headX + headR * 0.5f, eyeY, headX + headR * 0.8f, eyeY, 2.f);
        g.drawLine(headX + headR * 1.2f, eyeY, headX + headR * 1.5f, eyeY, 2.f);
    }
    else
    {
        g.fillEllipse(headX + headR * 0.5f, eyeY - 4.f, 8.f, 8.f);
        g.fillEllipse(headX + headR * 1.2f, eyeY - 4.f, 8.f, 8.f);
    }

    juce::Path smile;
    smile.addArc(headX + headR * 0.4f, eyeY + 6.f, headR * 1.2f, headR * 0.6f,
                 0.2f, juce::MathConstants<float>::pi - 0.2f);
    g.setColour(juce::Colours::darkred);
    g.strokePath(smile, juce::PathStrokeType(2.f));

    // 状態オーバーレイ
    g.setFont(juce::Font(22.f));
    switch (currentState_)
    {
        case AnimState::Clicked:
        case AnimState::Triggered:
            g.setColour(juce::Colours::gold.withAlpha(0.85f));
            g.drawText(juce::String::fromUTF8("\u266a"), getLocalBounds(), juce::Justification::topRight);
            break;
        case AnimState::Happy:
            g.setColour(juce::Colours::yellow.withAlpha(0.9f));
            g.drawText("*", getLocalBounds(), juce::Justification::topRight);
            break;
        case AnimState::Excited:
            g.setColour(juce::Colours::orangered.withAlpha(0.9f));
            g.drawText("!", getLocalBounds(), juce::Justification::topRight);
            break;
        case AnimState::Love:
            g.setColour(juce::Colours::hotpink.withAlpha(0.9f));
            g.drawText(juce::String::fromUTF8("\u2665"), getLocalBounds(), juce::Justification::topRight);
            break;
        case AnimState::Shy:
            g.setColour(juce::Colours::lightpink.withAlpha(0.8f));
            g.drawText("~", getLocalBounds(), juce::Justification::topRight);
            break;
        default: break;
    }
}
