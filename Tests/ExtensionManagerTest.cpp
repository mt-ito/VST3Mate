#include <JuceHeader.h>
#include "ExtensionManager.h"

class ExtensionManagerTests : public juce::UnitTest
{
public:
    ExtensionManagerTests() : juce::UnitTest("ExtensionManager", "VST3Mate") {}

    void runTest() override
    {
        beginTest("Default config has expected values");
        {
            ExtensionManager mgr;
            const auto& cfg = mgr.getConfig();
            expectEquals(cfg.scaleName, juce::String("Major"));
            expectEquals(cfg.rootNote, 60);
            expectEquals(cfg.minOctave, 3);
            expectEquals(cfg.maxOctave, 6);
            expectWithinAbsoluteError(cfg.minVelocity, 0.5f, 1e-5f);
            expectWithinAbsoluteError(cfg.maxVelocity, 1.0f, 1e-5f);
            expectEquals(cfg.oscSendHost, juce::String("127.0.0.1"));
            expectEquals(cfg.oscSendPort, 9000);
            expectEquals(cfg.oscReceivePort, 9001);
        }

        beginTest("loadFromDirectory creates config file if absent");
        {
            auto tmpDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("vst3mate_test_" + juce::String(juce::Time::currentTimeMillis()));
            tmpDir.createDirectory();

            ExtensionManager mgr;
            mgr.loadFromDirectory(tmpDir);

            auto cfgFile = tmpDir.getChildFile("config.json");
            expect(cfgFile.existsAsFile(), "config.json not created in empty directory");

            tmpDir.deleteRecursively();
        }

        beginTest("loadFromDirectory parses written config round-trip");
        {
            auto tmpDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("vst3mate_test_roundtrip_" + juce::String(juce::Time::currentTimeMillis()));
            tmpDir.createDirectory();

            {
                ExtensionManager writer;
                writer.loadFromDirectory(tmpDir);
                auto& cfg = writer.getConfigMutable();
                cfg.scaleName    = "Blues";
                cfg.rootNote     = 48;
                cfg.oscSendPort  = 7777;
                cfg.animFps      = 24;
                writer.saveConfig();
            }

            {
                ExtensionManager reader;
                reader.loadFromDirectory(tmpDir);
                const auto& cfg = reader.getConfig();
                expectEquals(cfg.scaleName,   juce::String("Blues"));
                expectEquals(cfg.rootNote,    48);
                expectEquals(cfg.oscSendPort, 7777);
                expectEquals(cfg.animFps,     24);
            }

            tmpDir.deleteRecursively();
        }

        beginTest("Custom scales are parsed from JSON");
        {
            auto tmpDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("vst3mate_test_custom_scales_" + juce::String(juce::Time::currentTimeMillis()));
            tmpDir.createDirectory();

            juce::String jsonStr = R"({
                "scaleName": "Major",
                "rootNote": 60,
                "minOctave": 3,
                "maxOctave": 6,
                "minVelocity": 0.5,
                "maxVelocity": 1.0,
                "minDuration": 0.25,
                "maxDuration": 1.0,
                "oscSendHost": "127.0.0.1",
                "oscSendPort": 9000,
                "oscReceivePort": 9001,
                "oscNoteAddress": "/mascot/note",
                "oscAnimAddress": "/mascot/anim",
                "oscStateAddress": "/mascot/state",
                "idleAnimDir": "idle",
                "clickAnimDir": "clicked",
                "animFps": 10,
                "customScales": [
                    { "name": "MyScale", "intervals": [0, 2, 5, 7, 10] }
                ]
            })";

            tmpDir.getChildFile("config.json").replaceWithText(jsonStr);

            ExtensionManager mgr;
            mgr.loadFromDirectory(tmpDir);

            const auto& scales = mgr.getCustomScalesRef();
            expectEquals((int)scales.size(), 1);
            expectEquals(scales[0].name, juce::String("MyScale"));
            expectEquals((int)scales[0].intervals.size(), 5);
            expectEquals(scales[0].intervals[2], 5);

            tmpDir.deleteRecursively();
        }

        beginTest("Custom scales with empty name or intervals are ignored");
        {
            auto tmpDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("vst3mate_test_invalid_scales_" + juce::String(juce::Time::currentTimeMillis()));
            tmpDir.createDirectory();

            juce::String jsonStr = R"({
                "scaleName": "Major",
                "rootNote": 60,
                "minOctave": 3,
                "maxOctave": 6,
                "minVelocity": 0.5,
                "maxVelocity": 1.0,
                "minDuration": 0.25,
                "maxDuration": 1.0,
                "oscSendHost": "127.0.0.1",
                "oscSendPort": 9000,
                "oscReceivePort": 9001,
                "oscNoteAddress": "/mascot/note",
                "oscAnimAddress": "/mascot/anim",
                "oscStateAddress": "/mascot/state",
                "idleAnimDir": "idle",
                "clickAnimDir": "clicked",
                "animFps": 10,
                "customScales": [
                    { "name": "",         "intervals": [0, 2, 4] },
                    { "name": "NoIntervals", "intervals": [] }
                ]
            })";

            tmpDir.getChildFile("config.json").replaceWithText(jsonStr);

            ExtensionManager mgr;
            mgr.loadFromDirectory(tmpDir);

            expect(mgr.getCustomScalesRef().empty(),
                   "Invalid custom scales should be ignored");

            tmpDir.deleteRecursively();
        }
    }
};

static ExtensionManagerTests extensionManagerTests;
