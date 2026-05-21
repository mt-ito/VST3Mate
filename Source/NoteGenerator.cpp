#include "NoteGenerator.h"
#include "ExtensionManager.h"

NoteGenerator::NoteGenerator()
{
    initBuiltinScales();
}

NoteGenerator::NoteGenerator(ExtensionManager* extManager)
    : extensionManager_(extManager)
{
    initBuiltinScales();
    if (extensionManager_)
    {
        for (auto& cs : extensionManager_->getCustomScalesRef())
            scales_.push_back({ cs.name, cs.intervals });
    }
}

void NoteGenerator::initBuiltinScales()
{
    scales_ = {
        { "Major",           { 0, 2, 4, 5, 7, 9, 11 } },
        { "Minor",           { 0, 2, 3, 5, 7, 8, 10 } },
        { "Pentatonic Major",{ 0, 2, 4, 7, 9 } },
        { "Pentatonic Minor",{ 0, 3, 5, 7, 10 } },
        { "Blues",           { 0, 3, 5, 6, 7, 10 } },
        { "Chromatic",       { 0,1,2,3,4,5,6,7,8,9,10,11 } },
        { "Whole Tone",      { 0, 2, 4, 6, 8, 10 } },
        { "Dorian",          { 0, 2, 3, 5, 7, 9, 10 } },
    };
}

NoteEvent NoteGenerator::generateRandom() const
{
    NoteEvent ev;
    ev.noteNumber = pickRandomNoteFromScale();
    ev.velocity   = minVelocity_ + rng_.nextFloat() * (maxVelocity_ - minVelocity_);
    ev.durationSeconds = minDuration_ + rng_.nextFloat() * (maxDuration_ - minDuration_);
    return ev;
}

int NoteGenerator::pickRandomNoteFromScale() const
{
    const auto& intervals = scales_[currentScaleIndex_].intervals;
    int octave = minOctave_ + rng_.nextInt(maxOctave_ - minOctave_ + 1);
    int intervalIdx = rng_.nextInt((int)intervals.size());
    int note = rootNote_ + (octave - 4) * 12 + intervals[intervalIdx];
    return juce::jlimit(0, 127, note);
}

void NoteGenerator::setScale(const juce::String& scaleName)
{
    for (int i = 0; i < (int)scales_.size(); ++i)
    {
        if (scales_[i].name.equalsIgnoreCase(scaleName))
        {
            currentScaleIndex_ = i;
            return;
        }
    }
}

void NoteGenerator::setRootNote(int midiRootNote) { rootNote_ = midiRootNote; }
void NoteGenerator::setOctaveRange(int minOct, int maxOct) { minOctave_ = minOct; maxOctave_ = maxOct; }
void NoteGenerator::setVelocityRange(float mn, float mx) { minVelocity_ = mn; maxVelocity_ = mx; }
void NoteGenerator::setDurationRange(float mn, float mx) { minDuration_ = mn; maxDuration_ = mx; }

juce::StringArray NoteGenerator::getAvailableScales() const
{
    juce::StringArray names;
    for (auto& s : scales_) names.add(s.name);
    return names;
}
