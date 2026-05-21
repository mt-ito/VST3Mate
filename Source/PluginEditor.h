#pragma once
#include <JuceHeader.h>
#include "CharacterComponent.h"
#include "BinarySpriteLoader.h"
#include "PluginProcessor.h"

class VST3MateEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit VST3MateEditor(VST3MateProcessor& processor);
    ~VST3MateEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildOscSettingsPanel();
    void applyOscSettings();
    void loadCharacterAssets();
    void setupOscCallbacks();
    void showNotePopup(int noteNumber);

    VST3MateProcessor& processor_;

    CharacterComponent character_;

    // --- OSC Settings panel ---
    juce::Label        oscSendHostLabel_   { {}, "Send Host:" };
    juce::TextEditor   oscSendHostEditor_;
    juce::Label        oscSendPortLabel_   { {}, "Send Port:" };
    juce::TextEditor   oscSendPortEditor_;
    juce::Label        oscRecvPortLabel_   { {}, "Recv Port:" };
    juce::TextEditor   oscRecvPortEditor_;
    juce::TextButton   oscApplyButton_     { "Apply OSC" };

    // --- Scale selector ---
    juce::Label        scaleLabel_         { {}, "Scale:" };
    juce::ComboBox     scaleCombo_;

    // --- Note popup ---
    juce::String       lastNoteText_;
    int                notePopupFrames_ = 0;

    // --- Status label ---
    juce::Label        statusLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST3MateEditor)
};
