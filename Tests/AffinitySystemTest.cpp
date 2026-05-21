#include <JuceHeader.h>
#include "AffinitySystem.h"

class AffinitySystemTests : public juce::UnitTest
{
public:
    AffinitySystemTests() : juce::UnitTest("AffinitySystem", "VST3Mate") {}

    void runTest() override
    {
        beginTest("Initial state is Neutral with 0 points");
        {
            AffinitySystem sys;
            expectEquals(sys.getPoints(), 0);
            expect(sys.getLevel() == AffinitySystem::Level::Neutral);
        }

        beginTest("addAffinity accumulates points correctly");
        {
            AffinitySystem sys;
            sys.addAffinity(100);
            expectEquals(sys.getPoints(), 100);
            sys.addAffinity(50);
            expectEquals(sys.getPoints(), 150);
        }

        beginTest("Points are clamped to [0, 1000]");
        {
            AffinitySystem sys;
            sys.addAffinity(2000);
            expectEquals(sys.getPoints(), 1000);
        }

        beginTest("Level thresholds: Neutral->Friend at 250");
        {
            AffinitySystem sys;
            sys.addAffinity(249);
            expect(sys.getLevel() == AffinitySystem::Level::Neutral);
            sys.addAffinity(1); // now at 250
            expect(sys.getLevel() == AffinitySystem::Level::Friend);
        }

        beginTest("Level thresholds: Friend->Close at 500");
        {
            AffinitySystem sys;
            sys.addAffinity(499);
            expect(sys.getLevel() == AffinitySystem::Level::Friend);
            sys.addAffinity(1); // now at 500
            expect(sys.getLevel() == AffinitySystem::Level::Close);
        }

        beginTest("Level thresholds: Close->Love at 750");
        {
            AffinitySystem sys;
            sys.addAffinity(749);
            expect(sys.getLevel() == AffinitySystem::Level::Close);
            sys.addAffinity(1); // now at 750
            expect(sys.getLevel() == AffinitySystem::Level::Love);
        }

        beginTest("Level callback fires on level change");
        {
            AffinitySystem sys;
            int callbackCount = 0;
            AffinitySystem::Level lastLevel = AffinitySystem::Level::Neutral;
            sys.setLevelCallback([&](AffinitySystem::Level l) {
                ++callbackCount;
                lastLevel = l;
            });
            sys.addAffinity(250); // Neutral -> Friend
            expectEquals(callbackCount, 1);
            expect(lastLevel == AffinitySystem::Level::Friend);
        }

        beginTest("Motion callback fires on level change");
        {
            AffinitySystem sys;
            juce::String lastMotion;
            sys.setMotionCallback([&](const juce::String& m) { lastMotion = m; });
            sys.addAffinity(250); // level up triggers motion
            expect(lastMotion.isNotEmpty(), "Motion callback not fired on level change");
        }

        beginTest("getProgress returns 0.0 at level start");
        {
            AffinitySystem sys;
            sys.addAffinity(250); // exactly at Friend start
            auto progress = sys.getProgress();
            expectWithinAbsoluteError(progress, 0.0f, 1e-4f);
        }

        beginTest("getProgress returns ~0.5 at mid-level");
        {
            AffinitySystem sys;
            sys.addAffinity(375); // midpoint of Friend (250-499)
            auto progress = sys.getProgress();
            expectWithinAbsoluteError(progress, 0.5f, 0.02f);
        }

        beginTest("levelName returns correct strings");
        {
            expectEquals(AffinitySystem::levelName(AffinitySystem::Level::Neutral), juce::String("Neutral"));
            expectEquals(AffinitySystem::levelName(AffinitySystem::Level::Friend),  juce::String("Friend"));
            expectEquals(AffinitySystem::levelName(AffinitySystem::Level::Close),   juce::String("Close"));
            expectEquals(AffinitySystem::levelName(AffinitySystem::Level::Love),    juce::String("Love"));
        }

        beginTest("levelThreshold returns correct values");
        {
            expectEquals(AffinitySystem::levelThreshold(AffinitySystem::Level::Neutral),   0);
            expectEquals(AffinitySystem::levelThreshold(AffinitySystem::Level::Friend),  250);
            expectEquals(AffinitySystem::levelThreshold(AffinitySystem::Level::Close),   500);
            expectEquals(AffinitySystem::levelThreshold(AffinitySystem::Level::Love),    750);
        }

        beginTest("saveToValueTree and loadFromValueTree round-trip");
        {
            AffinitySystem sys;
            sys.addAffinity(600); // Close level

            juce::ValueTree tree("AffinityState");
            sys.saveToValueTree(tree);

            AffinitySystem sys2;
            sys2.loadFromValueTree(tree);
            expectEquals(sys2.getPoints(), sys.getPoints());
            expect(sys2.getLevel() == sys.getLevel());
        }

        beginTest("Loaded state with 0 points defaults to Neutral");
        {
            juce::ValueTree tree("AffinityState");
            AffinitySystem sys;
            sys.loadFromValueTree(tree);
            expectEquals(sys.getPoints(), 0);
            expect(sys.getLevel() == AffinitySystem::Level::Neutral);
        }
    }
};

static AffinitySystemTests affinitySystemTests;
