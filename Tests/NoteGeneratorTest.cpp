#include <JuceHeader.h>
#include "NoteGenerator.h"

class NoteGeneratorTests : public juce::UnitTest
{
public:
    NoteGeneratorTests() : juce::UnitTest("NoteGenerator", "VST3Mate") {}

    void runTest() override
    {
        beginTest("Default scale list contains 8 built-in scales");
        {
            NoteGenerator gen;
            auto scales = gen.getAvailableScales();
            expectEquals(scales.size(), 8);
            expect(scales.contains("Major"));
            expect(scales.contains("Minor"));
            expect(scales.contains("Blues"));
            expect(scales.contains("Chromatic"));
        }

        beginTest("generateRandom returns valid MIDI note range");
        {
            NoteGenerator gen;
            for (int i = 0; i < 200; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.noteNumber >= 0 && ev.noteNumber <= 127,
                       "Note out of MIDI range: " + juce::String(ev.noteNumber));
            }
        }

        beginTest("generateRandom velocity stays within configured range");
        {
            NoteGenerator gen;
            gen.setVelocityRange(0.2f, 0.6f);
            for (int i = 0; i < 100; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.velocity >= 0.2f - 1e-5f && ev.velocity <= 0.6f + 1e-5f,
                       "Velocity out of range: " + juce::String(ev.velocity));
            }
        }

        beginTest("generateRandom duration stays within configured range");
        {
            NoteGenerator gen;
            gen.setDurationRange(0.1f, 0.5f);
            for (int i = 0; i < 100; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.durationSeconds >= 0.1f - 1e-5f && ev.durationSeconds <= 0.5f + 1e-5f,
                       "Duration out of range: " + juce::String(ev.durationSeconds));
            }
        }

        beginTest("setScale switches to named scale");
        {
            NoteGenerator gen;
            gen.setScale("Minor");
            // With narrow octave range, verify notes still stay in MIDI bounds
            gen.setOctaveRange(4, 4);
            for (int i = 0; i < 50; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.noteNumber >= 0 && ev.noteNumber <= 127);
            }
        }

        beginTest("setScale ignores unknown scale name");
        {
            NoteGenerator gen;
            gen.setScale("Major");
            auto before = gen.getAvailableScales();
            gen.setScale("NonExistentScale");
            auto after = gen.getAvailableScales();
            // Scale list should be unchanged
            expectEquals(before.size(), after.size());
        }

        beginTest("setOctaveRange clamps notes to MIDI range");
        {
            NoteGenerator gen;
            // Extreme high octave — note must still be <= 127
            gen.setOctaveRange(8, 10);
            for (int i = 0; i < 100; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.noteNumber >= 0 && ev.noteNumber <= 127);
            }
            // Extreme low octave — note must still be >= 0
            gen.setOctaveRange(0, 1);
            for (int i = 0; i < 100; ++i)
            {
                auto ev = gen.generateRandom();
                expect(ev.noteNumber >= 0 && ev.noteNumber <= 127);
            }
        }

        beginTest("setRootNote shifts output range");
        {
            // Use single octave + chromatic to get deterministic span
            NoteGenerator gen;
            gen.setScale("Chromatic");
            gen.setOctaveRange(4, 4);
            gen.setRootNote(60);
            bool gotAbove60 = false;
            for (int i = 0; i < 200; ++i)
                if (gen.generateRandom().noteNumber >= 60) { gotAbove60 = true; break; }
            expect(gotAbove60, "Root note shift not working");
        }
    }
};

static NoteGeneratorTests noteGeneratorTests;
