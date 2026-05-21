#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

class ExtensionManager
{
public:
    struct Config
    {
        juce::String scaleName     = "Major";
        int    rootNote            = 60;
        int    minOctave           = 3;
        int    maxOctave           = 6;
        float  minVelocity         = 0.5f;
        float  maxVelocity         = 1.0f;
        float  minDuration         = 0.25f;
        float  maxDuration         = 1.0f;

        juce::String oscSendHost   = "127.0.0.1";
        int    oscSendPort         = 9000;
        int    oscReceivePort      = 9001;

        juce::String oscNoteAddress  = "/mascot/note";
        juce::String oscAnimAddress  = "/mascot/anim";
        juce::String oscStateAddress = "/mascot/state";

        juce::String idleAnimDir   = "idle";
        juce::String clickAnimDir  = "clicked";
        int    animFps             = 10;
    };

    struct CustomScale
    {
        juce::String name;
        std::vector<int> intervals;
    };

    ExtensionManager();
    ~ExtensionManager();

    void loadFromDirectory(const juce::File& dir);
    void saveConfig();
    const Config& getConfig() const { return config_; }
    Config& getConfigMutable()      { return config_; }

    const std::vector<CustomScale>& getCustomScalesRef() const { return customScales_; }

    // Change listener for hot-reload
    using ChangeCallback = std::function<void()>;
    void setChangeCallback(ChangeCallback cb) { changeCallback_ = std::move(cb); }

private:
    void parseConfigJson(const juce::var& json);
    juce::var buildConfigJson() const;

    Config config_;
    std::vector<CustomScale> customScales_;
    juce::File configFile_;
    ChangeCallback changeCallback_;

    // File watcher via polling
    juce::Time lastModTime_;
    std::unique_ptr<juce::Timer> watchTimer_;
};
