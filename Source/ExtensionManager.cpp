#include "ExtensionManager.h"
#include "NoteGenerator.h"

// Minimal polling timer for file watch
struct WatchTimer : public juce::Timer
{
    std::function<void()> callback;
    void timerCallback() override { if (callback) callback(); }
};

ExtensionManager::ExtensionManager()
{
    auto timer = std::make_unique<WatchTimer>();
    timer->callback = [this]()
    {
        if (configFile_.existsAsFile())
        {
            auto mt = configFile_.getLastModificationTime();
            if (mt != lastModTime_)
            {
                lastModTime_ = mt;
                auto result = juce::JSON::parse(configFile_.loadFileAsString());
                if (result.isObject())
                {
                    parseConfigJson(result);
                    if (changeCallback_) changeCallback_();
                }
            }
        }
    };
    timer->startTimer(2000); // check every 2s
    watchTimer_ = std::move(timer);
}

ExtensionManager::~ExtensionManager()
{
    if (watchTimer_) watchTimer_->stopTimer();
}

void ExtensionManager::loadFromDirectory(const juce::File& dir)
{
    configFile_ = dir.getChildFile("config.json");
    if (configFile_.existsAsFile())
    {
        auto result = juce::JSON::parse(configFile_.loadFileAsString());
        if (result.isObject())
            parseConfigJson(result);
        lastModTime_ = configFile_.getLastModificationTime();
    }
    else
    {
        // Write defaults
        saveConfig();
    }
}

void ExtensionManager::saveConfig()
{
    if (!configFile_.getParentDirectory().exists())
        configFile_.getParentDirectory().createDirectory();

    auto json = buildConfigJson();
    configFile_.replaceWithText(juce::JSON::toString(json, true));
}

void ExtensionManager::parseConfigJson(const juce::var& j)
{
    auto get = [&](const char* key, auto defaultVal) -> decltype(defaultVal)
    {
        auto v = j[key];
        if constexpr (std::is_same_v<decltype(defaultVal), juce::String>)
            return v.isVoid() ? defaultVal : v.toString();
        else if constexpr (std::is_same_v<decltype(defaultVal), float>)
            return v.isVoid() ? defaultVal : (float)v;
        else
            return v.isVoid() ? defaultVal : (int)v;
    };

    config_.scaleName      = get("scaleName",      config_.scaleName);
    config_.rootNote       = get("rootNote",        config_.rootNote);
    config_.minOctave      = get("minOctave",       config_.minOctave);
    config_.maxOctave      = get("maxOctave",       config_.maxOctave);
    config_.minVelocity    = get("minVelocity",     config_.minVelocity);
    config_.maxVelocity    = get("maxVelocity",     config_.maxVelocity);
    config_.minDuration    = get("minDuration",     config_.minDuration);
    config_.maxDuration    = get("maxDuration",     config_.maxDuration);
    config_.oscSendHost    = get("oscSendHost",     config_.oscSendHost);
    config_.oscSendPort    = get("oscSendPort",     config_.oscSendPort);
    config_.oscReceivePort = get("oscReceivePort",  config_.oscReceivePort);
    config_.oscNoteAddress = get("oscNoteAddress",  config_.oscNoteAddress);
    config_.oscAnimAddress = get("oscAnimAddress",  config_.oscAnimAddress);
    config_.oscStateAddress= get("oscStateAddress", config_.oscStateAddress);
    config_.idleAnimDir    = get("idleAnimDir",     config_.idleAnimDir);
    config_.clickAnimDir   = get("clickAnimDir",    config_.clickAnimDir);
    config_.animFps        = get("animFps",         config_.animFps);

    // Custom scales
    customScales_.clear();
    auto* scalesArr = j["customScales"].getArray();
    if (scalesArr)
    {
        for (auto& sv : *scalesArr)
        {
            CustomScale cs;
            cs.name = sv["name"].toString();
            auto* ia = sv["intervals"].getArray();
            if (ia)
                for (auto& iv : *ia)
                    cs.intervals.push_back((int)iv);
            if (!cs.name.isEmpty() && !cs.intervals.empty())
                customScales_.push_back(std::move(cs));
        }
    }
}

juce::var ExtensionManager::buildConfigJson() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("scaleName",       config_.scaleName);
    obj->setProperty("rootNote",        config_.rootNote);
    obj->setProperty("minOctave",       config_.minOctave);
    obj->setProperty("maxOctave",       config_.maxOctave);
    obj->setProperty("minVelocity",     config_.minVelocity);
    obj->setProperty("maxVelocity",     config_.maxVelocity);
    obj->setProperty("minDuration",     config_.minDuration);
    obj->setProperty("maxDuration",     config_.maxDuration);
    obj->setProperty("oscSendHost",     config_.oscSendHost);
    obj->setProperty("oscSendPort",     config_.oscSendPort);
    obj->setProperty("oscReceivePort",  config_.oscReceivePort);
    obj->setProperty("oscNoteAddress",  config_.oscNoteAddress);
    obj->setProperty("oscAnimAddress",  config_.oscAnimAddress);
    obj->setProperty("oscStateAddress", config_.oscStateAddress);
    obj->setProperty("idleAnimDir",     config_.idleAnimDir);
    obj->setProperty("clickAnimDir",    config_.clickAnimDir);
    obj->setProperty("animFps",         config_.animFps);

    juce::Array<juce::var> customScalesArr;
    for (auto& cs : customScales_)
    {
        juce::DynamicObject::Ptr s = new juce::DynamicObject();
        s->setProperty("name", cs.name);
        juce::Array<juce::var> ivArr;
        for (int iv : cs.intervals) ivArr.add(iv);
        s->setProperty("intervals", ivArr);
        customScalesArr.add(s.get());
    }
    obj->setProperty("customScales", customScalesArr);

    return juce::var(obj.get());
}
