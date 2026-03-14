/*
 * Ewisynth audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier: GPLv3
 *
 * Copyright (C) 2026 Nils Brederlow <dingodoppelt@jazz-polizei.de>
 */

#ifndef PLUGIN_EWISYNTH_H
#define PLUGIN_EWISYNTH_H

#include "DistrhoPlugin.hpp"
#include "polyfotz.h"
#include "variableshapeoscillator.h"
#include "pitchtracker.h"
#include <cstdint>

#define MAX_POLYPHONY 16

START_NAMESPACE_DISTRHO

#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif

#ifndef CLAMP
#define CLAMP(v, min, max) (MIN((max), MAX((min), (v))))
#endif

#ifndef DB_CO
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#endif

// -----------------------------------------------------------------------

class PluginEwisynth : public Plugin {
public:
    enum Parameters {
        CONTROL_TUNE = 0,
        CONTROL_OCTAVE,
        CONTROL_TRANSPOSE,
        CONTROL_GAIN,
        CONTROL_LEVEL,
        CONTROL_SLEWTIME,
        CONTROL_ARPRANGE,
        CONTROL_ARPTIME,
        CONTROL_POLYPHONY,
        CONTROL_DETUNE,
        CONTROL_BANK,
        CONTROL_VOICING,
        CONTROL_ROTATOR,
        CONTROL_PHASE,
        CONTROL_SHAPE,
        CONTROL_PRESSURE,
        CONTROL_CURVE,
        CONTROL_USEAUDIO,
        CONTROL_USEPOLYFOTZ,
        CONTROL_SENSITIVITY,
        CONTROL_THRESHOLD,
        CONTROL_NR
    };

    PluginEwisynth();

    ~PluginEwisynth();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override {
        return "Ewisynth";
    }

    const char* getDescription() const override {
        return "simple synth for breath controllers";
    }

    const char* getMaker() const noexcept override {
        return "NB";
    }

    const char* getHomePage() const override {
        return "https://jazz-polizei.de/plugins/ewisynth";
    }

    const char* getLicense() const noexcept override {
        return "https://spdx.org/licenses/GPLv3";
    }

    uint32_t getVersion() const noexcept override {
        return d_version(0, 1, 0);
    }

    // Go to:
    //
    // http://service.steinberg.de/databases/plugin.nsf/plugIn
    //
    // Get a proper plugin UID and fill it in here!
    int64_t getUniqueId() const noexcept override {
        return d_cconst('a', 'b', 'c', 'd');
    }

    // -------------------------------------------------------------------
    // Init

    void initAudioPort(bool input, uint32_t index, AudioPort& port);
    void initParameter(uint32_t index, Parameter& parameter) override;
    void initProgramName(uint32_t index, String& programName) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;
    void loadProgram(uint32_t index) override;

    // -------------------------------------------------------------------
    // Optional

    // Optional callback to inform the plugin about a sample rate change.
    void sampleRateChanged(double newSampleRate) override;

    // -------------------------------------------------------------------
    // Process

    void activate() override;

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override;


    // -------------------------------------------------------------------

private:
    float           fParams[CONTROL_NR];
    double          fSampleRate;

    float currFrequency = 440.f;
    float targetFrequency = 440.f;
    float realFrequency = 440.f;
    float freqRatio() { return currFrequency / targetFrequency; }
    float slewSteps = 0.f;
    float slewStepsRemaining = 0.f;
    float exponent() { return (slewSteps > 0) ? slewStepsRemaining / slewSteps : 1.f; }
    float pitchFactor() { return powf(freqRatio(), exponent()); }
    float currBendFactor = 1.f;
    float currPulseWidth = .5f;
    float currPressure = 0.f;
    float currShape = 0.f;
    float lastPhase = 0.f;
    VariableShapeOscillator SAWosc[MAX_POLYPHONY];
    VariableShapeOscillator SQRosc[MAX_POLYPHONY];
    PolyFotz polyfotz;
    struct StereoPair {
        float sqr_l = 0.f;
        float saw_r = 0.f;
    };
    StereoPair sumOscillators();
    float waveshaper(float sample) {
        return 2/(1+exp(-2*sample))-1;
    }
    struct Arpeggiator {
        bool isActive = false;
        uint8_t range = 0;
        uint8_t index = 1;
        int8_t indexIncrement = 1;
        uint32_t arpStepsInSamples = 8000;
        uint32_t arpStepsRemaining = arpStepsInSamples;
        void advance() {
            if (!isActive) return;

            if (index >= range) {
                index = range;
                indexIncrement = -1;
            }
            if (index <= 0) {
                index = 0;
                indexIncrement = 1;
            }
            if (arpStepsInSamples > 0) {
                if (arpStepsRemaining > 0) {
                    arpStepsRemaining--;
                } else {
                    arpStepsRemaining = arpStepsInSamples;
                    index += indexIncrement;
                }
            }
        }
        int getIndex(int voicingSize) {
            return index % voicingSize;
        }
        int getOctave(int voicingSize) {
            int octave = index / voicingSize;
            if (range > 0 || arpStepsInSamples > 0) octave -= 1;
            return octave;
        }
    } arpeggiator;

    PitchTracker* pt;


    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEwisynth)
};

struct Preset {
    const char* name;
    float params[PluginEwisynth::CONTROL_NR];
};

const Preset factoryPresets[] = {
    // {
    //     "Unity Gain",
    //     {0.0f}
    // }
    //,{
    //    "Another preset",  // preset name
    //    {-14.0f, ...}      // array of presetCount float param values
    //}
};

const uint presetCount = sizeof(factoryPresets) / sizeof(Preset);

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // #ifndef PLUGIN_EWISYNTH_H
