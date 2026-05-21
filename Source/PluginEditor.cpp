#include "PluginEditor.h"

static juce::String midiNoteToName(int note)
{
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String(names[note % 12]) + juce::String(note / 12 - 1);
}

VST3MateEditor::VST3MateEditor(VST3MateProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(400, 560);
    setResizable(true, true);
    setResizeLimits(300, 440, 800, 960);

    // Character
    addAndMakeVisible(character_);
    character_.setClickCallback([this]()
    {
        auto ev = processor_.triggerRandomNote();
        showNotePopup(ev.noteNumber);
    });

    // OSC settings
    buildOscSettingsPanel();

    // Scale selector
    addAndMakeVisible(scaleLabel_);
    addAndMakeVisible(scaleCombo_);
    auto scales = processor_.getNoteGenerator().getAvailableScales();
    for (int i = 0; i < scales.size(); ++i)
        scaleCombo_.addItem(scales[i], i + 1);
    scaleCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
    scaleCombo_.onChange = [this]()
    {
        auto name = scaleCombo_.getText();
        processor_.getNoteGenerator().setScale(name);
        processor_.getExtensionManager().getConfigMutable().scaleName = name;
    };

    // Affinity label
    addAndMakeVisible(affinityLabel_);
    affinityLabel_.setJustificationType(juce::Justification::centredLeft);
    affinityLabel_.setFont(juce::Font(11.f));

    // Status
    addAndMakeVisible(statusLabel_);
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setFont(juce::Font(12.f));

    loadCharacterAssets();
    setupOscCallbacks();
    setupAffinityCallbacks();

    startTimer(50); // 20fps for UI updates
}

VST3MateEditor::~VST3MateEditor()
{
    stopTimer();
}

void VST3MateEditor::buildOscSettingsPanel()
{
    const auto& cfg = processor_.getExtensionManager().getConfig();

    auto setupEditor = [&](juce::TextEditor& ed, const juce::String& text)
    {
        addAndMakeVisible(ed);
        ed.setText(text, false);
        ed.setJustification(juce::Justification::centredLeft);
    };

    addAndMakeVisible(oscSendHostLabel_);
    setupEditor(oscSendHostEditor_, cfg.oscSendHost);

    addAndMakeVisible(oscSendPortLabel_);
    setupEditor(oscSendPortEditor_, juce::String(cfg.oscSendPort));

    addAndMakeVisible(oscRecvPortLabel_);
    setupEditor(oscRecvPortEditor_, juce::String(cfg.oscReceivePort));

    addAndMakeVisible(oscApplyButton_);
    oscApplyButton_.onClick = [this]() { applyOscSettings(); };
}

void VST3MateEditor::applyOscSettings()
{
    auto& cfg = processor_.getExtensionManager().getConfigMutable();
    cfg.oscSendHost    = oscSendHostEditor_.getText();
    cfg.oscSendPort    = oscSendPortEditor_.getText().getIntValue();
    cfg.oscReceivePort = oscRecvPortEditor_.getText().getIntValue();

    processor_.getOscHandler().connectSender(cfg.oscSendHost, cfg.oscSendPort);
    processor_.getOscHandler().startReceiving(cfg.oscReceivePort,
                                               cfg.oscNoteAddress,
                                               cfg.oscAnimAddress,
                                               cfg.oscStateAddress);
    processor_.getExtensionManager().saveConfig();

    bool ok = processor_.getOscHandler().isSenderConnected();
    statusLabel_.setText(ok ? "OSC connected" : "OSC connection failed",
                         juce::dontSendNotification);
}

void VST3MateEditor::loadCharacterAssets()
{
    const auto& cfg = processor_.getExtensionManager().getConfig();
    character_.setFps(cfg.animFps);

    // 1. バイナリデータ（Assets/sprites/）からロード（デフォルト）
    auto idleFrames    = BinarySpriteLoader::loadIdleFrames();
    auto clickedFrames = BinarySpriteLoader::loadClickedFrames();
    auto happyFrames   = BinarySpriteLoader::loadByKeyword("happy_");
    auto excitedFrames = BinarySpriteLoader::loadByKeyword("excited_");
    auto loveFrames    = BinarySpriteLoader::loadByKeyword("love_");
    auto shyFrames     = BinarySpriteLoader::loadByKeyword("shy_");

    // 2. AppData のカスタムスプライトがあればそちらで上書き
    auto extDir     = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("VST3Mate").getChildFile("extensions");
    auto idleDir    = extDir.getChildFile("sprites").getChildFile(cfg.idleAnimDir);
    auto clickedDir = extDir.getChildFile("sprites").getChildFile(cfg.clickAnimDir);

    if (idleDir.isDirectory() && idleDir.getNumberOfChildFiles(juce::File::findFiles, "*.png") > 0)
        character_.loadAnimations(idleDir, clickedDir);
    else
        character_.setFrames(std::move(idleFrames), std::move(clickedFrames));

    // 好感度アニメーションは常にバイナリデータから（カスタム上書き未対応の状態）
    character_.setAffinityFrames(std::move(happyFrames),
                                  std::move(excitedFrames),
                                  std::move(loveFrames),
                                  std::move(shyFrames));
}

void VST3MateEditor::setupAffinityCallbacks()
{
    processor_.getAffinitySystem().setMotionCallback([this](const juce::String& motionName)
    {
        // AffinitySystem::Timer は message thread で発火するので callAsync 不要
        onMotionTriggered(motionName);
    });

    processor_.getAffinitySystem().setLevelCallback([this](AffinitySystem::Level newLevel)
    {
        auto name = AffinitySystem::levelName(newLevel);
        statusLabel_.setText("Level Up: " + name + "!", juce::dontSendNotification);
        repaint(); // affinity bar update
    });
}

void VST3MateEditor::onMotionTriggered(const juce::String& motionName)
{
    if (motionName == AffinitySystem::MOTION_SHY)
        character_.setAnimState(AnimState::Shy);
    else if (motionName == AffinitySystem::MOTION_HAPPY)
        character_.setAnimState(AnimState::Happy);
    else if (motionName == AffinitySystem::MOTION_EXCITED)
        character_.setAnimState(AnimState::Excited);
    else if (motionName == AffinitySystem::MOTION_LOVE)
        character_.setAnimState(AnimState::Love);
}

void VST3MateEditor::setupOscCallbacks()
{
    OscHandler::Callbacks cbs;

    cbs.onNoteReceived = [this](int note, float vel, float dur)
    {
        juce::MessageManager::callAsync([this, note, vel, dur]()
        {
            processor_.triggerNote(note, vel, dur);
            character_.setAnimState(AnimState::Triggered);
            showNotePopup(note);
        });
    };

    cbs.onAnimReceived = [this](const juce::String& animName)
    {
        juce::MessageManager::callAsync([this, animName]()
        {
            if (animName.equalsIgnoreCase("idle"))
                character_.setAnimState(AnimState::Idle);
            else
                character_.setAnimState(AnimState::Triggered);
        });
    };

    cbs.onStateReceived = [this](const juce::String& state)
    {
        juce::MessageManager::callAsync([this, state]()
        {
            statusLabel_.setText("OSC: " + state, juce::dontSendNotification);
        });
    };

    processor_.getOscHandler().setCallbacks(std::move(cbs));
}

void VST3MateEditor::showNotePopup(int noteNumber)
{
    lastNoteText_ = midiNoteToName(noteNumber);
    notePopupFrames_ = 30; // show for ~1.5s at 20fps
    repaint();
}

void VST3MateEditor::timerCallback()
{
    if (notePopupFrames_ > 0)
    {
        --notePopupFrames_;
        repaint();
    }

    // 好感度ラベルを定期更新
    auto& aff = processor_.getAffinitySystem();
    auto levelStr = AffinitySystem::levelName(aff.getLevel());
    affinityLabel_.setText(levelStr + "  " + juce::String(aff.getPoints()) + " / 1000",
                           juce::dontSendNotification);
}

void VST3MateEditor::paintAffinityBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& aff   = processor_.getAffinitySystem();
    float prog  = aff.getProgress();
    auto level  = aff.getLevel();

    // 色をレベルで変える
    juce::Colour barColour;
    switch (level)
    {
        case AffinitySystem::Level::Neutral: barColour = juce::Colour(0xff6b8cba); break;
        case AffinitySystem::Level::Friend:  barColour = juce::Colour(0xff6bba7f); break;
        case AffinitySystem::Level::Close:   barColour = juce::Colour(0xffffcc55); break;
        case AffinitySystem::Level::Love:    barColour = juce::Colour(0xffff6b9d); break;
    }

    auto r = bounds.toFloat();
    g.setColour(juce::Colour(0xff2e2e3e));
    g.fillRoundedRectangle(r, 4.f);
    g.setColour(barColour);
    g.fillRoundedRectangle(r.withWidth(r.getWidth() * prog), 4.f);
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawRoundedRectangle(r, 4.f, 1.f);
}

void VST3MateEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e2e));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.f, juce::Font::bold));
    g.drawText("VST3Mate", getLocalBounds().removeFromTop(28), juce::Justification::centred);

    // Note popup overlay
    if (notePopupFrames_ > 0 && lastNoteText_.isNotEmpty())
    {
        float alpha = (float)notePopupFrames_ / 30.f;
        g.setColour(juce::Colours::gold.withAlpha(alpha));
        g.setFont(juce::Font(32.f, juce::Font::bold));
        g.drawText(lastNoteText_,
                   character_.getBounds().translated(10, -20).withHeight(40),
                   juce::Justification::centred);
    }

    // Affinity bar (rendered in paint because it needs custom drawing)
    auto barBounds = affinityLabel_.getBounds().withLeft(affinityLabel_.getRight() + 4)
                                               .withRight(getWidth() - 10)
                                               .reduced(0, 4);
    paintAffinityBar(g, barBounds);
}

void VST3MateEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(30); // title

    // Character (square, top section)
    int charSize = juce::jmin(area.getWidth(), area.getHeight() / 2);
    character_.setBounds(area.removeFromTop(charSize).withSizeKeepingCentre(charSize, charSize));
    area.removeFromTop(6);

    // Affinity bar row
    auto affRow = area.removeFromTop(20);
    affinityLabel_.setBounds(affRow.removeFromLeft(120));
    // remaining space is drawn in paint() as the bar
    area.removeFromTop(4);

    // Scale selector
    auto scaleRow = area.removeFromTop(26);
    scaleLabel_.setBounds(scaleRow.removeFromLeft(60));
    scaleCombo_.setBounds(scaleRow);
    area.removeFromTop(4);

    // OSC settings
    auto labelW = 80;
    auto rowH   = 24;

    auto row1 = area.removeFromTop(rowH);
    oscSendHostLabel_.setBounds(row1.removeFromLeft(labelW));
    oscSendHostEditor_.setBounds(row1);
    area.removeFromTop(4);

    auto row2 = area.removeFromTop(rowH);
    oscSendPortLabel_.setBounds(row2.removeFromLeft(labelW));
    oscSendPortEditor_.setBounds(row2);
    area.removeFromTop(4);

    auto row3 = area.removeFromTop(rowH);
    oscRecvPortLabel_.setBounds(row3.removeFromLeft(labelW));
    oscRecvPortEditor_.setBounds(row3);
    area.removeFromTop(6);

    oscApplyButton_.setBounds(area.removeFromTop(28).withSizeKeepingCentre(100, 24));
    area.removeFromTop(4);

    statusLabel_.setBounds(area.removeFromTop(20));
}
