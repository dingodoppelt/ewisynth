/*
 * Ewisynth audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier: GPLv3
 *
 * Copyright (C) 2026 Nils Brederlow <dingodoppelt@jazz-polizei.de>
 */

#include "PluginEwisynth.hpp"
#include "MicrotrackerModel.h"
#include <cstdint>

#define AUBIOBUFSIZE 2048

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PluginEwisynth::PluginEwisynth()
    : Plugin(CONTROL_NR, presetCount, 0),  // paramCount param(s), presetCount program(s), 0 states
      pt(nullptr),
      ef(nullptr),
      filterL(nullptr),
      filterR(nullptr)
{
    const float sample_rate = getSampleRate();
    polyfotz.Init(MAX_POLYPHONY);

    for (unsigned p = 0; p < CONTROL_NR; ++p) {
        Parameter param;
        initParameter(p, param);
        setParameterValue(p, param.ranges.def);
    }
    for (int i = 0; i < MAX_POLYPHONY; i++) {
        SAWosc[i].Init(sample_rate);
        SQRosc[i].Init(sample_rate);
        SAWosc[i].SetWaveshape(0);
        SQRosc[i].SetWaveshape(0.f);
    }

    pt = new PitchTracker(sample_rate, AUBIOBUFSIZE);
    ef = new EnvelopeFollower();
    filterL = new MicrotrackerMoog(sample_rate);
    filterR = new MicrotrackerMoog(sample_rate);
}

PluginEwisynth::~PluginEwisynth() {
    delete pt;
    delete ef;
    delete filterL;
    delete filterR;
}

// -----------------------------------------------------------------------
// Init

void PluginEwisynth::initAudioPort(bool input, uint32_t index, AudioPort& port)
{
    if (input) {
        port.name = "Audio In";
        port.symbol = "audioIn";
        return;
    }
    
    switch (index)
    {
        case 0:
            port.name   = "Square Out";
            port.symbol = "sqrOut";
            break;
        case 1:
            port.name   = "Saw Out";
            port.symbol = "sawOut";
            break;
    }
}

void PluginEwisynth::initParameter(uint32_t index, Parameter& parameter) {
    if (index >= CONTROL_NR)
        return;

    switch (index) {
        case CONTROL_TUNE:
            parameter.name = "Tune";
            parameter.shortName = "Tune";
            parameter.symbol = "tune";
            parameter.ranges.def = -0.0f;
            parameter.ranges.min = -0.05f;
            parameter.ranges.max = 0.05f;
            parameter.unit = "cent";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_OCTAVE:
            parameter.name = "Octave";
            parameter.shortName = "Oct";
            parameter.symbol = "octave";
            parameter.ranges.def = 0;
            parameter.ranges.min = -3;
            parameter.ranges.max = 3;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_TRANSPOSE:
            parameter.name = "Transpose";
            parameter.shortName = "Transp";
            parameter.symbol = "transpose";
            parameter.ranges.def = 0;
            parameter.ranges.min = -12;
            parameter.ranges.max = 12;
            parameter.unit = "semitones";
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_GAIN:
            parameter.name = "Drive Gain";
            parameter.shortName = "DriveGain";
            parameter.symbol = "driveGain";
            parameter.ranges.def = 7.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 8.f;
            parameter.unit = "db";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_VOL_LEAD:
            parameter.name = "Lead Volume";
            parameter.shortName = "LeadVol";
            parameter.symbol = "leadVolume";
            parameter.ranges.def = 1.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.f;
            parameter.unit = "%";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_LEVEL:
            parameter.name = "Level";
            parameter.shortName = "Lvl";
            parameter.symbol = "level";
            parameter.ranges.def = 0.7f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.unit = "%";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_SLEWTIME:
            parameter.name = "Slewtime";
            parameter.shortName = "Slew";
            parameter.symbol = "slewtime";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 48000;
            parameter.unit = "frames";
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_ARPRANGE:
            parameter.name = "Arprange";
            parameter.shortName = "Arpr";
            parameter.symbol = "arprange";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 12;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_ARPTIME:
            parameter.name = "Arptime";
            parameter.shortName = "Arpt";
            parameter.symbol = "arptime";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 8000;
            parameter.unit = "frames";
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_POLYPHONY:
            parameter.name = "Polyphony";
            parameter.shortName = "Poly";
            parameter.symbol = "polyphony";
            parameter.ranges.def = 1;
            parameter.ranges.min = 1;
            parameter.ranges.max = 16;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_DETUNE:
            parameter.name = "Detune";
            parameter.shortName = "dtune";
            parameter.symbol = "detune";
            parameter.ranges.def = 0.5f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.unit = "cent";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_BANK:
            parameter.name = "Bank";
            parameter.shortName = "Bank";
            parameter.symbol = "bank";
            parameter.ranges.def = 2;
            parameter.ranges.min = 0;
            parameter.ranges.max = 5;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_VOICING:
            parameter.name = "Voicing";
            parameter.shortName = "Voicing";
            parameter.symbol = "voicing";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 15;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_ROTATOR:
            parameter.name = "Rotator";
            parameter.shortName = "Rotator";
            parameter.symbol = "rotator";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 2;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_PHASE:
            parameter.name = "Phase";
            parameter.shortName = "Phase";
            parameter.symbol = "phase";
            parameter.ranges.def = 0.2f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.unit = "%";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_SHAPE:
            parameter.name = "Shape";
            parameter.shortName = "Shape";
            parameter.symbol = "shape";
            parameter.ranges.def = 0.0f;
            parameter.ranges.min = -1.0f;
            parameter.ranges.max = 1.0f;
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_PRESSURE:
            parameter.name = "Pressure";
            parameter.shortName = "Press";
            parameter.symbol = "pressure";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            break;
        case CONTROL_CURVE:
            parameter.name = "Pressure Sensitivity";
            parameter.shortName = "PressSens";
            parameter.symbol = "pressSens";
            parameter.ranges.def = 0.5f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.0f;
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_SENSITIVITY:
            parameter.name = "Pitch Sensitivity";
            parameter.shortName = "PitSens";
            parameter.symbol = "pitchSensitivity";
            parameter.ranges.def = 60.0f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 60.0f;
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_THRESHOLD:
            parameter.name = "Pitch Threshold";
            parameter.shortName = "PitThres";
            parameter.symbol = "pitchThreshold";
            parameter.ranges.def = 80.0f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.0f;
            parameter.unit = "%";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_USEAUDIO:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Use Audio Input";
            parameter.symbol = "useAudioIn";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case CONTROL_USE_RMS:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Envelope Follower";
            parameter.symbol = "envFollow";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case CONTROL_RMS_LEN:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            parameter.name = "RMS Window Length";
            parameter.symbol = "rmsLen";
            parameter.ranges.def = 1024;
            parameter.ranges.min = getBufferSize();
            parameter.ranges.max = MAX_RMS_BUFFER;
            break;
        case CONTROL_USEPOLYFOTZ:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Toggle Harmony";
            parameter.symbol = "toggleHarmony";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case CONTROL_PRESS_CC:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            parameter.name = "MIDI Pressure CC";
            parameter.symbol = "pressCC";
            parameter.ranges.def = 96;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            break;
        case CONTROL_FILT_CUTOFF:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Filter Cutoff Frequency";
            parameter.symbol = "lpfCutoff";
            parameter.ranges.def = 0.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = (float)MAX_FILTER_CUTOFF;
            break;
        case CONTROL_FILT_RESO:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Filter Resonance";
            parameter.symbol = "filterReso";
            parameter.ranges.def = 0.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.f;
            break;
        case CONTROL_FILT_CURVE:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Pressure to Filter Cutoff Sens";
            parameter.symbol = "filterCurve";
            parameter.ranges.def = 0.5f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.f;
            break;
        case CONTROL_PW_CURVE:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Pulsewidth Sens";
            parameter.symbol = "pulseSens";
            parameter.ranges.def = 0.5f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.f;
            break;
        case CONTROL_POLY_PW_SCALE:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Poly Pulsewidth Scale";
            parameter.symbol = "polyPWScale";
            parameter.ranges.def = 0.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 1.f;
            break;
    }
}

/**
  Set the name of the program @a index.
  This function will be called once, shortly after the plugin is created.
*/
void PluginEwisynth::initProgramName(uint32_t index, String& programName) {
    if (index < presetCount) {
        programName = factoryPresets[index].name;
    }
}

// -----------------------------------------------------------------------
// Internal data

/**
  Optional callback to inform the plugin about a sample rate change.
*/
void PluginEwisynth::sampleRateChanged(double newSampleRate) {
    fSampleRate = newSampleRate;
}

/**
  Get the current value of a parameter.
*/
float PluginEwisynth::getParameterValue(uint32_t index) const {
    return fParams[index];
}

/**
  Change a parameter value.
*/
void PluginEwisynth::setParameterValue(uint32_t index, float value) {

    fParams[index] = value;
    
    switch (index) {
        case CONTROL_TUNE:
            polyfotz.setTune(value);
            break;
        case CONTROL_OCTAVE:
            polyfotz.setOctave((int8_t)value);
            break;
        case CONTROL_TRANSPOSE:
            polyfotz.setTranspose((int8_t)value);
            break;
        case CONTROL_ARPRANGE:
            arpeggiator.range = (uint32_t)value;
            break;
        case CONTROL_ARPTIME:
            arpeggiator.arpStepsInSamples = (uint32_t)value;
            break;
        case CONTROL_BANK:
            polyfotz.setBank((uint8_t)value);
            break;
        case CONTROL_VOICING:
            polyfotz.setVoicing((uint8_t)value);
            break;
        case CONTROL_ROTATOR:
            polyfotz.setRotator((uint8_t)value);
            break;
        case CONTROL_DETUNE:
            polyfotz.setDetune(value);
            break;
        case CONTROL_SHAPE:
            currShape = value;
            break;
        case CONTROL_POLYPHONY:
            polyfotz.setPolyphony((uint8_t)value);
            break;
        case CONTROL_SLEWTIME:
            slewSteps = value;
            break;
        case CONTROL_PRESS_CC:
            pressureCC = (uint8_t)value;
            break;
        case CONTROL_SENSITIVITY:
            if (pt != nullptr) pt->sensitivity = value;
            break;
        case CONTROL_THRESHOLD:
            if (pt != nullptr) pt->threshold = value;
            break;
        case CONTROL_USEPOLYFOTZ:
            if (value > .5) {
                polyfotz.setPitchbend(0); // pitchbend negative
            } else {
                polyfotz.setPitchbend(8192); // pitchbend neutral
            }
            break;
        case CONTROL_CURVE:
            // fall through to next case so take the correct value
            [[fallthrough]];
        case CONTROL_PW_CURVE:
            value = getParameterValue(CONTROL_PRESSURE);
            // fall through to next case so take the correct value
            [[fallthrough]];
        case CONTROL_PRESSURE:
            currPressure = getCurve(value, 127.f, getParameterValue(CONTROL_CURVE), false);
            currPulseWidth = getCurve(value, 127.f, getParameterValue(CONTROL_PW_CURVE), false) / 2.f + .5f; // limit pulse width to .5 - 1.
            break;
        case CONTROL_RMS_LEN:
            if (ef != nullptr) ef->init((uint16_t)value);
            break;
        case CONTROL_FILT_CURVE:
            value = getParameterValue(CONTROL_FILT_CUTOFF);
            // fall through to next case so take the correct value
            [[fallthrough]];
        case CONTROL_FILT_CUTOFF:
            if (filterL != nullptr) filterL->SetCutoff(getCurve(value, MAX_FILTER_CUTOFF, getParameterValue(CONTROL_FILT_CURVE), true));
            if (filterR != nullptr) filterR->SetCutoff(getCurve(value, MAX_FILTER_CUTOFF, getParameterValue(CONTROL_FILT_CURVE), true));
            break;
        case CONTROL_FILT_RESO:
            if (filterL != nullptr) filterL->SetResonance(value);
            if (filterR != nullptr) filterR->SetResonance(value);
            break;
        default:
            break;
    }
}

/**
  Load a program.
  The host may call this function from any context,
  including realtime processing.
*/
void PluginEwisynth::loadProgram(uint32_t index) {
    if (index < presetCount) {
        for (int i=0; i < CONTROL_NR; i++) {
            setParameterValue(i, factoryPresets[index].params[i]);
        }
    }
}

// -----------------------------------------------------------------------
// Process

void PluginEwisynth::activate() {
    // plugin is activated
}

void PluginEwisynth::run(const float** inputs, float** outputs,
                         uint32_t frames,
                         const MidiEvent* midiEvents, uint32_t midiEventCount) {

    const float normalizedCutoff = getParameterValue(CONTROL_FILT_CUTOFF) / (float)MAX_FILTER_CUTOFF;
    // get the left and right audio outputs
    float* const outL = outputs[0];
    float* const outR = outputs[1];
    float currPitch[2];
    uint32_t  offset = 0;

    pt->processBlock(inputs, currPitch, frames);
    if (getParameterValue(CONTROL_USEAUDIO) && currPitch[1] > .5f) {
        currFrequency = realFrequency;
        if (polyfotz.setFrequency(currPitch[0])) {
            // The note playing is different from the previous one
            // send note_off for last note
            writeMidiEvent(packEvent(0x90, polyfotz.getLastNote(), 0, offset));
            // send note on for new note
            writeMidiEvent(packEvent(0x90, polyfotz.getNote(), (uint8_t)(currPressure * 127.f), offset));
        };
        targetFrequency = polyfotz.getFrequency(0);
        slewStepsRemaining = slewSteps;
    }

    for (uint32_t i=0; i<midiEventCount; i++) {
        if (midiEvents[i].size <= 3)
        {
            // MIDI in
            uint8_t status = midiEvents[i].data[0];
            uint8_t byte1 = midiEvents[i].data[1] & 127;
            
            for (uint32_t j = offset; j <= midiEvents[i].frame; j++) {
                if (getParameterValue(CONTROL_USE_RMS)) {
                    const float rms = ef->update(inputs[0][j]);
                    currPressure = getCurve(rms, 1.f, getParameterValue(CONTROL_CURVE), false);
                    currPulseWidth = getCurve(rms, 1.f, getParameterValue(CONTROL_PW_CURVE), false) / 2.f + .5f; // limit pulse width to .5 - 1.
                    filterL->SetCutoff((getCurve(rms, 1.f - normalizedCutoff, getParameterValue(CONTROL_FILT_CURVE), true) + normalizedCutoff) * (float)MAX_FILTER_CUTOFF);
                    filterR->SetCutoff((getCurve(rms, 1.f - normalizedCutoff, getParameterValue(CONTROL_FILT_CURVE), true) + normalizedCutoff) * (float)MAX_FILTER_CUTOFF);
                }
                const StereoPair outputs = sumOscillators();
                outL[j] = outputs.sqr_l;
                outR[j] = outputs.saw_r;
                // TODO: send harmony and arpeggiator notes over MIDI out
                offset++;
            }
            // MIDI in
            switch (status & 0xf0) {
                case 0x90:
                    currFrequency = realFrequency;
                    polyfotz.setNote(byte1);
                    targetFrequency = polyfotz.getFrequency(0);
                    slewStepsRemaining = slewSteps;
                    break;
                case 0xE0:
                    polyfotz.setPitchbend(midiEvents[i].data[2] << 7 | midiEvents[i].data[1]); // 2^( ((pitchbend - 8192) / 8192 * bendrange = 2 / max_pitchbend = 16383) / 12 )
                    break;
                default:
                    break;
            }
        }
    }

    // envelope follower

    for (uint32_t j = offset; j < frames; j++) {
        if ((bool)getParameterValue(CONTROL_USE_RMS)) {
            const float rms = ef->update(inputs[0][j]);
            currPressure = getCurve(rms, 1.f, getParameterValue(CONTROL_CURVE), false);
            currPulseWidth = getCurve(rms, 1.f, getParameterValue(CONTROL_PW_CURVE), false) / 2.f + .5f; // limit pulse width to .5 - 1.
            filterL->SetCutoff((getCurve(rms, 1.f - normalizedCutoff, getParameterValue(CONTROL_FILT_CURVE), true) + normalizedCutoff) * (float)MAX_FILTER_CUTOFF);
            filterR->SetCutoff((getCurve(rms, 1.f - normalizedCutoff, getParameterValue(CONTROL_FILT_CURVE), true) + normalizedCutoff) * (float)MAX_FILTER_CUTOFF);
            writeMidiEvent(packEvent(0xB0, pressureCC, (uint8_t)(currPressure * 127.f), j));
        }
        const StereoPair outputs = sumOscillators();
        outL[j] = outputs.sqr_l;
        outR[j] = outputs.saw_r;
    }
    filterL->Process(outL, frames);
    filterR->Process(outR, frames);
}

// -----------------------------------------------------------------------

Plugin* createPlugin() {
    return new PluginEwisynth();
}

// -----------------------------------------------------------------------

PluginEwisynth::StereoPair PluginEwisynth::sumOscillators() {
    StereoPair out;
    float delta = 0.f;
    float phase_ = getParameterValue(CONTROL_PHASE);
    float level_ = getParameterValue(CONTROL_LEVEL);
    float lead_lvl_ = getParameterValue(CONTROL_VOL_LEAD);
    uint8_t poly_ = (uint8_t)getParameterValue(CONTROL_POLYPHONY);
    float polyPWScale_ = -getParameterValue(CONTROL_POLY_PW_SCALE) / poly_;
    
    if (phase_ != lastPhase) {
        delta = lastPhase - phase_;
        lastPhase = phase_;
    };
    
    int voicingSize = polyfotz.getActiveVoicingSize();
    realFrequency = polyfotz.getFrequency(0) * pitchFactor();
    for (int i = 0; i < poly_; i++) {
        float freq;
        arpeggiator.isActive = poly_ == 1 && polyfotz.isPitchbendNegative();
        if (arpeggiator.isActive) {
            freq = polyfotz.getFrequency(arpeggiator.getIndex(voicingSize)) * pow(2, -arpeggiator.getOctave(voicingSize)) * pitchFactor();
            for (int j = 0; j < voicingSize; j++) {
                SAWosc[j+1].SetFreq(polyfotz.getFrequency(j) * pitchFactor());
                SQRosc[j+1].SetFreq(polyfotz.getFrequency(j) * pitchFactor());
                SQRosc[j+1].SetWaveshape(currShape);
                SAWosc[j+1].SetWaveshape(currShape);
                SAWosc[j+1].SetPW(currPulseWidth);
                SQRosc[j+1].SetPW(currPulseWidth);
                if (delta != 0.f) SQRosc[j+1].OffsetPhase(delta);
                out.sqr_l += SQRosc[j+1].Process() / voicingSize * currPressure;
                out.saw_r += SAWosc[j+1].Process() / voicingSize * currPressure;
            }
        } else {
            freq = polyfotz.getFrequency(i) * pitchFactor();
        }
        SAWosc[i].SetFreq(freq);
        SQRosc[i].SetFreq(freq);
        SQRosc[i].SetWaveshape(currShape);
        SAWosc[i].SetWaveshape(currShape);
        SAWosc[i].SetPW(((polyPWScale_ * i + 1.f) * currPulseWidth) / 2.f + .5f);
        SQRosc[i].SetPW(((polyPWScale_ * i + 1.f) * currPulseWidth)  / 2.f + .5f);
        if (delta != 0.f) SQRosc[i].OffsetPhase(delta);
        out.sqr_l += SQRosc[i].Process() / poly_ * currPressure * lead_lvl_;
        out.saw_r += SAWosc[i].Process() / poly_ * currPressure * lead_lvl_;
    }
    out.sqr_l = waveshaper(out.sqr_l) * level_;
    out.saw_r = waveshaper(out.saw_r) * level_;
    (slewStepsRemaining > 0) ? slewStepsRemaining-- : currFrequency = targetFrequency;
    arpeggiator.advance();
    return out;
}

END_NAMESPACE_DISTRHO
