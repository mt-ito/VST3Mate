#include "OscHandler.h"

OscHandler::OscHandler()
{
    receiver_.addListener(this);
}

OscHandler::~OscHandler()
{
    stopReceiving();
    disconnectSender();
}

bool OscHandler::startReceiving(int port, const juce::String& noteAddr,
                                const juce::String& animAddr,
                                const juce::String& stateAddr)
{
    stopReceiving();

    noteAddress_  = noteAddr;
    animAddress_  = animAddr;
    stateAddress_ = stateAddr;

    receiverStarted_ = receiver_.connect(port);
    return receiverStarted_;
}

void OscHandler::stopReceiving()
{
    if (receiverStarted_)
    {
        receiver_.disconnect();
        receiverStarted_ = false;
    }
}

bool OscHandler::connectSender(const juce::String& host, int port)
{
    disconnectSender();
    senderConnected_ = sender_.connect(host, port);
    return senderConnected_;
}

void OscHandler::disconnectSender()
{
    if (senderConnected_)
    {
        sender_.disconnect();
        senderConnected_ = false;
    }
}

void OscHandler::sendNote(int noteNumber, float velocity, float duration,
                          const juce::String& address)
{
    if (!senderConnected_) return;
    sender_.send(juce::OSCAddressPattern(address),
                 (int32_t)noteNumber,
                 velocity,
                 duration);
}

void OscHandler::sendAnim(const juce::String& animName, const juce::String& address)
{
    if (!senderConnected_) return;
    sender_.send(juce::OSCAddressPattern(address), animName);
}

void OscHandler::sendState(const juce::String& state, const juce::String& address)
{
    if (!senderConnected_) return;
    sender_.send(juce::OSCAddressPattern(address), state);
}

void OscHandler::oscMessageReceived(const juce::OSCMessage& msg)
{
    auto addr = msg.getAddressPattern().toString();

    if (addr == noteAddress_ && callbacks_.onNoteReceived)
    {
        int   note     = msg.size() > 0 && msg[0].isInt32() ? msg[0].getInt32() : 60;
        float velocity = msg.size() > 1 && msg[1].isFloat32() ? msg[1].getFloat32() : 0.8f;
        float dur      = msg.size() > 2 && msg[2].isFloat32() ? msg[2].getFloat32() : 0.5f;
        callbacks_.onNoteReceived(note, velocity, dur);
    }
    else if (addr == animAddress_ && callbacks_.onAnimReceived)
    {
        juce::String name = msg.size() > 0 && msg[0].isString() ? msg[0].getString() : "idle";
        callbacks_.onAnimReceived(name);
    }
    else if (addr == stateAddress_ && callbacks_.onStateReceived)
    {
        juce::String state = msg.size() > 0 && msg[0].isString() ? msg[0].getString() : "";
        callbacks_.onStateReceived(state);
    }
}
