#pragma once
#include <JuceHeader.h>

struct NoteEvent
{
    int noteNumber;
    float velocity;
    float durationSeconds;
};

class ExtensionManager;

class NoteGenerator
{
public:
    struct ScaleDef
    {
        juce::String name;
        std::vector<int> intervals;
    };

    NoteGenerator();
    explicit NoteGenerator(ExtensionManager* extManager);

    NoteEvent generateRandom() const;

    void setScale(const juce::String& scaleName);
    void setRootNote(int midiRootNote);
    void setOctaveRange(int minOctave, int maxOctave);
    void setVelocityRange(float minVel, float maxVel);
    void setDurationRange(float minSec, float maxSec);

    juce::StringArray getAvailableScales() const;

private:
    void initBuiltinScales();
    int pickRandomNoteFromScale() const;

    std::vector<ScaleDef> scales_;
    int currentScaleIndex_ = 0;
    int rootNote_ = 60;
    int minOctave_ = 3;
    int maxOctave_ = 6;
    float minVelocity_ = 0.5f;
    float maxVelocity_ = 1.0f;
    float minDuration_ = 0.25f;
    float maxDuration_ = 1.0f;

    ExtensionManager* extensionManager_ = nullptr;
    mutable juce::Random rng_;
};
