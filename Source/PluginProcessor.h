#pragma once
#include <JuceHeader.h>
#include "NoteGenerator.h"
#include "OscHandler.h"
#include "ExtensionManager.h"
#include "AffinitySystem.h"

struct PendingNote
{
    int   noteNumber;
    float velocity;
    int   durationSamples;
    int   samplesPlayed = 0;
    bool  noteOnSent   = false;
};

class VST3MateProcessor : public juce::AudioProcessor
{
public:
    VST3MateProcessor();
    ~VST3MateProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VST3Mate"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Called from the editor when the character is clicked
    NoteEvent triggerRandomNote();

    // Called from OSC receive
    void triggerNote(int noteNumber, float velocity, float durationSec);

    NoteGenerator&    getNoteGenerator()    { return noteGenerator_; }
    OscHandler&       getOscHandler()       { return oscHandler_; }
    ExtensionManager& getExtensionManager() { return extensionManager_; }
    AffinitySystem&   getAffinitySystem()   { return affinitySystem_; }

    void applyConfigFromExtensions();

private:
    void enqueuePendingNote(int note, float vel, float durationSec);

    ExtensionManager extensionManager_;
    NoteGenerator    noteGenerator_;
    OscHandler       oscHandler_;
    AffinitySystem   affinitySystem_;

    juce::Array<PendingNote> pendingNotes_;
    juce::CriticalSection    noteLock_;

    double currentSampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST3MateProcessor)
};
