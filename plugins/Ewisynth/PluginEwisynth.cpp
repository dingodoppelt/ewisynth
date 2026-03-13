/*
 * Ewisynth audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier: GPLv3
 *
 * Copyright (C) 2026 Nils Brederlow <dingodoppelt@jazz-polizei.de>
 */

#include "PluginEwisynth.hpp"
#include <cstdint>

#define AUBIOBUFSIZE 2048

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PluginEwisynth::PluginEwisynth()
    : Plugin(CONTROL_NR, presetCount, 0),  // paramCount param(s), presetCount program(s), 0 states
      pt(nullptr)
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

}

PluginEwisynth::~PluginEwisynth() {
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
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_TRANSPOSE:
            parameter.name = "Transpose";
            parameter.shortName = "Transp";
            parameter.symbol = "transpose";
            parameter.ranges.def = 0;
            parameter.ranges.min = -12;
            parameter.ranges.max = 12;
            parameter.unit = "semitones";
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_GAIN:
            parameter.name = "Gain";
            parameter.shortName = "Gain";
            parameter.symbol = "gain";
            parameter.ranges.def = 7.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 8.f;
            parameter.unit = "db";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_LEVEL:
            parameter.name = "Level";
            parameter.shortName = "Lvl";
            parameter.symbol = "level";
            parameter.ranges.def = 0.7f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.unit = "percent";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_SLEWTIME:
            parameter.name = "Slewtime";
            parameter.shortName = "Slew";
            parameter.symbol = "slewtime";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 6000;
            parameter.unit = "frames";
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_ARPRANGE:
            parameter.name = "Arprange";
            parameter.shortName = "Arpr";
            parameter.symbol = "arprange";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 12;
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_ARPTIME:
            parameter.name = "Arptime";
            parameter.shortName = "Arpt";
            parameter.symbol = "arptime";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 8000;
            parameter.unit = "frames";
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_POLYPHONY:
            parameter.name = "Polyphony";
            parameter.shortName = "Poly";
            parameter.symbol = "polyphony";
            parameter.ranges.def = 1;
            parameter.ranges.min = 1;
            parameter.ranges.max = 16;
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
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
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_VOICING:
            parameter.name = "Voicing";
            parameter.shortName = "Voicing";
            parameter.symbol = "voicing";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 15;
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_ROTATOR:
            parameter.name = "Rotator";
            parameter.shortName = "Rotator";
            parameter.symbol = "rotator";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 2;
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_PHASE:
            parameter.name = "Phase";
            parameter.shortName = "Phase";
            parameter.symbol = "phase";
            parameter.ranges.def = 0.2f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.unit = "percent";
            parameter.hints = kParameterIsAutomatable;
            break;
        case CONTROL_SHAPE:
            parameter.name = "Shape";
            parameter.shortName = "Shape";
            parameter.symbol = "shape";
            parameter.ranges.def = 0.0f;
            parameter.ranges.min = 0.0f;
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
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            break;
        case CONTROL_CURVE:
            parameter.name = "Curve";
            parameter.shortName = "Curve";
            parameter.symbol = "curve";
            parameter.ranges.def = 1.0f;
            parameter.ranges.min = 1.0f;
            parameter.ranges.max = 10.0f;
            parameter.hints = kParameterIsAutomatable;
            break;
        case paramUseAudio:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Use Audio Input";
            parameter.symbol = "useAudioIn";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case paramUsePolyfotz:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Toggle Harmony";
            parameter.symbol = "toggleHarmony";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
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
            slewSteps = (uint8_t)value;
            break;
        case paramUsePolyfotz:
            if (value > .5) {
                polyfotz.setPitchbend(0); // pitchbend negative
            } else {
                polyfotz.setPitchbend(8192); // pitchbend neutral
            }
            break;
        case CONTROL_PRESSURE:
            currPressure = pow(value / 127.f, getParameterValue(CONTROL_CURVE));
            currPulseWidth = currPressure / 2.f + .5f; // limit pulse width to .5 - 1.
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

    // get the left and right audio outputs
    float* const outL = outputs[0];
    float* const outR = outputs[1];

    float currPitch[2];
    pt->processBlock(inputs, currPitch, frames);
    if (getParameterValue(paramUseAudio) && currPitch[1] > .5f) {
        currFrequency = realFrequency;
        polyfotz.setFrequency(currPitch[0]);
        targetFrequency = polyfotz.getFrequency(0);
        slewStepsRemaining = slewSteps;
        polyfotz.updateRotator();
    }
    uint32_t  offset = 0;

    for (uint32_t i=0; i<midiEventCount; i++) {
        if (midiEvents[i].size <= 3)
        {
            uint8_t status = midiEvents[i].data[0];
            uint8_t byte1 = midiEvents[i].data[1] & 127;
            
            for (uint32_t j = offset; j <= midiEvents[i].frame; j++) {
                const StereoPair outputs = sumOscillators();
                outL[j] = outputs.sqr_l;
                outR[j] = outputs.saw_r;
                offset++;
            }
            switch (status & 0xf0) {
                case 0x90:
                    currFrequency = realFrequency;
                    polyfotz.setNote(byte1);
                    targetFrequency = polyfotz.getFrequency(0);
                    slewStepsRemaining = slewSteps;
                    polyfotz.updateRotator();
                    break;
                case 0xE0:
                    polyfotz.setPitchbend(midiEvents[i].data[2] << 7 | midiEvents[i].data[1]); // 2^( ((pitchbend - 8192) / 8192 * bendrange = 2 / max_pitchbend = 16383) / 12 )
                    break;
                default:
                    break;
            }
        }
    }
    for (uint32_t j = offset; j < frames; j++) {
        const StereoPair outputs = sumOscillators();
        outL[j] = outputs.sqr_l;
        outR[j] = outputs.saw_r;
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin() {
    return new PluginEwisynth();
}

// -----------------------------------------------------------------------

PluginEwisynth::StereoPair PluginEwisynth::sumOscillators() {
  StereoPair out;
  uint8_t poly_ = (uint8_t)getParameterValue(CONTROL_POLYPHONY);
  float delta = 0.f;
  float phase_ = getParameterValue(CONTROL_PHASE);
  float gain_ = getParameterValue(CONTROL_GAIN);
  float level_ = getParameterValue(CONTROL_LEVEL);
  
  if (phase_ != lastPhase) {
    delta = lastPhase - phase_;
    lastPhase = phase_;
  };

  int voicingSize = polyfotz.getActiveVoicingSize();
  realFrequency = polyfotz.getFrequency(0) * pitchFactor();
  for (int i = 0; i < poly_; i++) {
    float freq;
    arpeggiator.isActive = (uint8_t)getParameterValue(CONTROL_POLYPHONY) == 1 && polyfotz.isPitchbendNegative();
    if (arpeggiator.isActive) {
      freq = polyfotz.getFrequency(arpeggiator.getIndex(voicingSize)) * pow(2, -arpeggiator.getOctave(voicingSize));
      for (int j = 0; j < voicingSize; j++) {
        SAWosc[j+1].SetFreq(polyfotz.getFrequency(j));
        SQRosc[j+1].SetFreq(polyfotz.getFrequency(j));
        SAWosc[j+1].SetPW(currPulseWidth);
        if (delta != 0.f) SQRosc[j+1].OffsetPhase(delta);
        if (currShape == 1.f) {
          SQRosc[j+1].SetWaveshape( 1.5f - currPulseWidth );
          SQRosc[j+1].SetPW(.5f);
        } else {
          SQRosc[j+1].SetWaveshape(currShape);
          SQRosc[j+1].SetPW(currPulseWidth);
        }
        out.sqr_l +=
            SQRosc[j+1].Process() / voicingSize * gain_ * currPressure;
        out.saw_r +=
            SAWosc[j+1].Process() / voicingSize * gain_ * currPressure;
      }
    } else {
      freq = polyfotz.getFrequency(i) * pitchFactor();
    }
    SAWosc[i].SetFreq(freq);
    SQRosc[i].SetFreq(freq);
    SAWosc[i].SetPW(currPulseWidth);
    if (delta != 0.f) SQRosc[i].OffsetPhase(delta);
    if (currShape == 1.f) {
      SQRosc[i].SetWaveshape( 1.5f - currPulseWidth );
      SQRosc[i].SetPW(.5f);
    } else {
      SQRosc[i].SetWaveshape(currShape);
      SQRosc[i].SetPW(currPulseWidth);
    }
    out.sqr_l +=
        SQRosc[i].Process() / poly_ * gain_ * currPressure;
    out.saw_r +=
        SAWosc[i].Process() / poly_ * gain_ * currPressure;
  }
  out.sqr_l = waveshaper(out.sqr_l) * level_;
  out.saw_r = waveshaper(out.saw_r) * level_;
  (slewStepsRemaining > 0) ? slewStepsRemaining-- : currFrequency = targetFrequency;
  arpeggiator.advance();
  return out;
}

END_NAMESPACE_DISTRHO
