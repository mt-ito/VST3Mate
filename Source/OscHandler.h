#pragma once
#include <JuceHeader.h>
#include "NoteGenerator.h"

class OscHandler : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    struct Callbacks
    {
        std::function<void(int noteNumber, float velocity, float duration)> onNoteReceived;
        std::function<void(const juce::String& animName)>                  onAnimReceived;
        std::function<void(const juce::String& state)>                     onStateReceived;
    };

    OscHandler();
    ~OscHandler();

    bool startReceiving(int port, const juce::String& noteAddr,
                        const juce::String& animAddr,
                        const juce::String& stateAddr);
    void stopReceiving();

    bool connectSender(const juce::String& host, int port);
    void disconnectSender();

    void sendNote(int noteNumber, float velocity, float duration,
                  const juce::String& address = "/mascot/note");
    void sendAnim(const juce::String& animName,
                  const juce::String& address = "/mascot/anim");
    void sendState(const juce::String& state,
                   const juce::String& address = "/mascot/state");

    void setCallbacks(Callbacks cbs) { callbacks_ = std::move(cbs); }

    bool isSenderConnected() const { return senderConnected_; }
    bool isReceiving() const       { return receiverStarted_; }

private:
    void oscMessageReceived(const juce::OSCMessage& msg) override;

    juce::OSCSender   sender_;
    juce::OSCReceiver receiver_;
    Callbacks         callbacks_;

    juce::String noteAddress_  = "/mascot/note";
    juce::String animAddress_  = "/mascot/anim";
    juce::String stateAddress_ = "/mascot/state";

    bool senderConnected_ = false;
    bool receiverStarted_ = false;
};
