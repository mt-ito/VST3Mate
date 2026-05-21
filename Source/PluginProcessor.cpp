#include "PluginProcessor.h"
#include "PluginEditor.h"

VST3MateProcessor::VST3MateProcessor()
    : AudioProcessor(BusesProperties()),
      noteGenerator_(&extensionManager_)
{
    // Load extensions config from user app data dir
    auto extDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("VST3Mate").getChildFile("extensions");
    extensionManager_.loadFromDirectory(extDir);
    extensionManager_.setChangeCallback([this]() { applyConfigFromExtensions(); });

    applyConfigFromExtensions();
}

VST3MateProcessor::~VST3MateProcessor()
{
    oscHandler_.stopReceiving();
    oscHandler_.disconnectSender();
}

void VST3MateProcessor::applyConfigFromExtensions()
{
    const auto& cfg = extensionManager_.getConfig();

    noteGenerator_.setScale(cfg.scaleName);
    noteGenerator_.setRootNote(cfg.rootNote);
    noteGenerator_.setOctaveRange(cfg.minOctave, cfg.maxOctave);
    noteGenerator_.setVelocityRange(cfg.minVelocity, cfg.maxVelocity);
    noteGenerator_.setDurationRange(cfg.minDuration, cfg.maxDuration);

    oscHandler_.connectSender(cfg.oscSendHost, cfg.oscSendPort);
    oscHandler_.startReceiving(cfg.oscReceivePort,
                               cfg.oscNoteAddress,
                               cfg.oscAnimAddress,
                               cfg.oscStateAddress);

    // Wire OSC callbacks
    OscHandler::Callbacks cbs;
    cbs.onNoteReceived = [this](int note, float vel, float dur)
    {
        triggerNote(note, vel, dur);
    };
    // anim/state callbacks are handled in the editor
    oscHandler_.setCallbacks(cbs);
}

void VST3MateProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate_ = sampleRate;
    juce::ScopedLock lock(noteLock_);
    pendingNotes_.clear();
}

void VST3MateProcessor::releaseResources() {}

void VST3MateProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    juce::ScopedLock lock(noteLock_);

    for (auto& pn : pendingNotes_)
    {
        if (!pn.noteOnSent)
        {
            midiMessages.addEvent(
                juce::MidiMessage::noteOn(1, pn.noteNumber,
                                         (uint8_t)juce::roundToInt(pn.velocity * 127.f)), 0);
            pn.noteOnSent = true;
        }
        pn.samplesPlayed += buffer.getNumSamples();
        if (pn.samplesPlayed >= pn.durationSamples)
        {
            midiMessages.addEvent(
                juce::MidiMessage::noteOff(1, pn.noteNumber), buffer.getNumSamples() - 1);
        }
    }

    // Remove finished notes
    pendingNotes_.removeIf([](const PendingNote& pn)
    {
        return pn.noteOnSent && pn.samplesPlayed >= pn.durationSamples;
    });
}

NoteEvent VST3MateProcessor::triggerRandomNote()
{
    auto ev = noteGenerator_.generateRandom();
    triggerNote(ev.noteNumber, ev.velocity, ev.durationSeconds);

    const auto& cfg = extensionManager_.getConfig();
    oscHandler_.sendNote(ev.noteNumber, ev.velocity, ev.durationSeconds, cfg.oscNoteAddress);
    oscHandler_.sendState("clicked", cfg.oscStateAddress);

    // クリックするたびに好感度 +5
    affinitySystem_.addAffinity(5);

    return ev;
}

void VST3MateProcessor::triggerNote(int noteNumber, float velocity, float durationSec)
{
    enqueuePendingNote(noteNumber, velocity, durationSec);
}

void VST3MateProcessor::enqueuePendingNote(int note, float vel, float durationSec)
{
    PendingNote pn;
    pn.noteNumber       = note;
    pn.velocity         = vel;
    pn.durationSamples  = juce::roundToInt(durationSec * currentSampleRate_);
    pn.samplesPlayed    = 0;
    pn.noteOnSent       = false;

    juce::ScopedLock lock(noteLock_);
    pendingNotes_.add(pn);
}

juce::AudioProcessorEditor* VST3MateProcessor::createEditor()
{
    return new VST3MateEditor(*this);
}

void VST3MateProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    extensionManager_.saveConfig();

    juce::ValueTree state("VST3MateState");
    affinitySystem_.saveToValueTree(state);

    juce::MemoryOutputStream stream(dest, false);
    state.writeToStream(stream);
}

void VST3MateProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData(data, (size_t)sizeInBytes);
    if (state.isValid())
        affinitySystem_.loadFromValueTree(state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VST3MateProcessor();
}
