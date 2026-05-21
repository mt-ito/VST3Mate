#include "PluginEditor.h"

static juce::String midiNoteToName(int note)
{
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String(names[note % 12]) + juce::String(note / 12 - 1);
}

VST3MateEditor::VST3MateEditor(VST3MateProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(400, 520);
    setResizable(true, true);
    setResizeLimits(300, 400, 800, 900);

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

    // Status
    addAndMakeVisible(statusLabel_);
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setFont(juce::Font(12.f));

    loadCharacterAssets();
    setupOscCallbacks();

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

    // 1. まずバイナリデータ（Assets/sprites/）からロード（デフォルト）
    auto idleFrames    = BinarySpriteLoader::loadIdleFrames();
    auto clickedFrames = BinarySpriteLoader::loadClickedFrames();

    // 2. AppData のカスタムスプライトがあればそちらで上書き
    auto extDir     = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("VST3Mate").getChildFile("extensions");
    auto idleDir    = extDir.getChildFile("sprites").getChildFile(cfg.idleAnimDir);
    auto clickedDir = extDir.getChildFile("sprites").getChildFile(cfg.clickAnimDir);

    if (idleDir.isDirectory() && idleDir.getNumberOfChildFiles(juce::File::findFiles, "*.png") > 0)
        character_.loadAnimations(idleDir, clickedDir);
    else
        character_.setFrames(std::move(idleFrames), std::move(clickedFrames));
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
}

void VST3MateEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(30); // title

    // Character (square, top section)
    int charSize = juce::jmin(area.getWidth(), area.getHeight() / 2);
    character_.setBounds(area.removeFromTop(charSize).withSizeKeepingCentre(charSize, charSize));
    area.removeFromTop(6);

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
