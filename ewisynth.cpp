/* include libs */
#include "lv2.h"
#include "curves.h"
#include "variableshapeoscillator.h"
#include "polyfotz.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2_util.h>
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>
#include <math.h>
#include <sys/types.h>

#define MAX_POLYPHONY 16

enum ControlPorts {
  CONTROL_TUNE = 0,
  CONTROL_OCTAVE = 1,
  CONTROL_TRANSPOSE = 2,
  CONTROL_GAIN = 3,
  CONTROL_LEVEL = 4,
  CONTROL_CURVEY = 5,
  CONTROL_CURVE = 6,
  CONTROL_POLYPHONY = 7,
  CONTROL_DETUNE = 8,
  CONTROL_BANK = 9,
  CONTROL_VOICING = 10,
  CONTROL_ROTATOR = 11,
  CONTROL_PHASE = 12,
  CONTROL_SHAPE = 13,
  CONTROL_NR = 14
};

enum PortGroups {
  PORT_MIDI_IN = 0,
  PORT_L_AUDIO_OUT = 1,
  PORT_R_AUDIO_OUT = 2,
  PORT_CONTROL = 3,
  PORT_NR = 4
};

struct Urids {
  LV2_URID midi_MidiEvent;
};

/* class definition */
class EwiSynth {
private:
  const LV2_Atom_Sequence *midi_in_ptr;
  float *l_audio_out_ptr;
  float *r_audio_out_ptr;
  const float *control_ptr[CONTROL_NR];
  LV2_URID_Map *map;
  Urids urids;
  double rate;
  float currFrequency = 440.f;
  float currBendFactor = 1.f;
  float currPulseWidth = .5f;
  float currPressure = .0f;
  float currShape = 1.f;
  float lastPhase = 0.f;
  VariableShapeOscillator SAWosc[MAX_POLYPHONY];
  VariableShapeOscillator SQRosc[MAX_POLYPHONY];
  void handleNoteOn(uint8_t note);
  void handlePressure(const uint8_t pressure);
  void handlePitchbend(const uint16_t pitchbend);
  void handleController(const uint8_t controller, const uint8_t value);
  void updateControls();
  PolyFotz polyfotz;
  Curves curve;
  struct StereoPair {
    float sqr_l = 0.f;
    float saw_r = 0.f;
  };
  StereoPair sumOscillators();
  float waveshaper(float sample) {
    return 2/(1+exp(-2*sample))-1;
  }

public:
  EwiSynth(const double sample_rate, const LV2_Feature *const *features);
  void connectPort(const uint32_t port, void *data_location);
  void run(const uint32_t sample_count);
};

EwiSynth::EwiSynth(const double sample_rate, const LV2_Feature *const *features)
    : midi_in_ptr(nullptr), l_audio_out_ptr(nullptr), r_audio_out_ptr(nullptr),
      control_ptr{nullptr}, map(nullptr), rate(sample_rate) {
  const char *missing =
      lv2_features_query(features, LV2_URID__map, &map, true, NULL);

  if (missing)
    throw;

  polyfotz.Init(MAX_POLYPHONY);
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    SAWosc[i].Init(sample_rate);
    SQRosc[i].Init(sample_rate);
    SAWosc[i].SetWaveshape(0);
    SQRosc[i].SetWaveshape(currShape);
  }

  urids.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
}

void EwiSynth::connectPort(const uint32_t port, void *data_location) {
  switch (port) {
  case PORT_MIDI_IN:
    midi_in_ptr = (const LV2_Atom_Sequence *)data_location;
    break;

  case PORT_L_AUDIO_OUT:
    l_audio_out_ptr = (float *)data_location;
    break;

  case PORT_R_AUDIO_OUT:
    r_audio_out_ptr = (float *)data_location;
    break;

  default:
    if (port < PORT_CONTROL + CONTROL_NR) {
      control_ptr[port - PORT_CONTROL] = (const float *)data_location;
    }
    break;
  }
}

void EwiSynth::run(const uint32_t sample_count) {
//   /* check if all ports connected */
//   if ((!midi_in_ptr)) return;
//
//   for (int i = 0; i < CONTROL_NR; ++i) {
//     if (!control_ptr[i])
//       return;
//   }
  updateControls();

  uint32_t  offset = 0;

  /* analyze incomming MIDI data */
  LV2_ATOM_SEQUENCE_FOREACH(midi_in_ptr, ev) {
    for (int i = offset; i < ev->time.frames; i++) {
      const StereoPair outputs = sumOscillators();
      l_audio_out_ptr[i] = outputs.sqr_l;
      r_audio_out_ptr[i] = outputs.saw_r;
    }
    if (ev->body.type == urids.midi_MidiEvent) {
      const uint8_t *const msg = (const uint8_t *)(ev + 1);
      const uint8_t typ = lv2_midi_message_type(msg);

      switch (typ) {
      case LV2_MIDI_MSG_NOTE_ON:
        handleNoteOn(msg[1]);
        break;

      case LV2_MIDI_MSG_CHANNEL_PRESSURE:
        handlePressure(msg[1]);
        break;

      case LV2_MIDI_MSG_CONTROLLER:
        handleController(msg[1], msg[2]);
        break;

      case LV2_MIDI_MSG_BENDER:
        handlePitchbend((msg[2] << 7) | msg[1]);
        break;

      default:
        break;
      }
    }
    offset = (uint32_t)ev->time.frames;
  }
  for (int i = offset; i < sample_count; i++) {
    const StereoPair outputs = sumOscillators();
    l_audio_out_ptr[i] = outputs.sqr_l;
    r_audio_out_ptr[i] = outputs.saw_r;
  }
}

EwiSynth::StereoPair EwiSynth::sumOscillators() {
  StereoPair out;
  int poly_ = *control_ptr[CONTROL_POLYPHONY];
  float delta = 0.f;

  if (*control_ptr[CONTROL_PHASE] != lastPhase) {
    delta = lastPhase - *control_ptr[CONTROL_PHASE];
    lastPhase = *control_ptr[CONTROL_PHASE];
  };

  for (int i = 0; i < poly_; i++) {
    SAWosc[i].SetFreq(polyfotz.getFrequency(i));
    SQRosc[i].SetFreq(polyfotz.getFrequency(i));
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
        SQRosc[i].Process() / poly_ * *control_ptr[CONTROL_GAIN] * currPressure;
    out.saw_r +=
        SAWosc[i].Process() / poly_ * *control_ptr[CONTROL_GAIN] * currPressure;
  }
  out.sqr_l = waveshaper(out.sqr_l) * *control_ptr[CONTROL_LEVEL];
  out.saw_r = waveshaper(out.saw_r) * *control_ptr[CONTROL_LEVEL];
  return out;
}

void EwiSynth::updateControls() {
  curve.setY(*control_ptr[CONTROL_CURVEY]);
  polyfotz.setTranspose(*control_ptr[CONTROL_TRANSPOSE]);
  polyfotz.setOctave(*control_ptr[CONTROL_OCTAVE]);
  polyfotz.setTune(*control_ptr[CONTROL_TUNE]);
  polyfotz.setBank(*control_ptr[CONTROL_BANK]);
  polyfotz.setVoicing(*control_ptr[CONTROL_VOICING]);
  polyfotz.setRotator(*control_ptr[CONTROL_ROTATOR]);
  polyfotz.setPolyphony(*control_ptr[CONTROL_POLYPHONY]);
  polyfotz.setDetune(*control_ptr[CONTROL_DETUNE]);
  if (*control_ptr[CONTROL_SHAPE] != currShape) currShape = *control_ptr[CONTROL_SHAPE];
}

void EwiSynth::handleNoteOn(const uint8_t note) {
  polyfotz.setNote(note);
  polyfotz.updateRotator();
}

void EwiSynth::handlePressure(const uint8_t pressure) {
  currPressure = curve.apply((float)pressure / 128.f);
  currPulseWidth = currPressure / 2.f + .5f; // limit pulse width to .5 - 1.
}

void EwiSynth::handlePitchbend(const uint16_t pitchbend) {
  polyfotz.setPitchbend(pitchbend); // 2^( ((pitchbend - 8192) / 8192 * bendrange = 2 / max_pitchbend = 16383) / 12 )
}

void EwiSynth::handleController(const uint8_t controller, const uint8_t value) {
  switch (controller) {
  default:
    break;
  }
}
/* internal core methods */
static LV2_Handle instantiate(const struct LV2_Descriptor *descriptor,
                              double sample_rate, const char *bundle_path,
                              const LV2_Feature *const *features) {
  EwiSynth *m = new EwiSynth(sample_rate, features);
  return m;
}

static void connect_port(LV2_Handle instance, uint32_t port,
                         void *data_location) {
  EwiSynth *m = (EwiSynth *)instance;
  if (m)
    m->connectPort(port, data_location);
}

static void activate(LV2_Handle instance) { /* not needed here */
}

static void run(LV2_Handle instance, uint32_t sample_count) {
  EwiSynth *m = (EwiSynth *)instance;
  if (m)
    m->run(sample_count);
}

static void deactivate(LV2_Handle instance) { /* not needed here */
}

static void cleanup(LV2_Handle instance) {
  EwiSynth *m = (EwiSynth *)instance;
  if (m)
    delete m;
}

static const void *extension_data(const char *uri) { return NULL; }

/* descriptor */
static LV2_Descriptor const descriptor = {
    "https://github.com/dingodoppelt/ewisynth",
    instantiate,
    connect_port,
    activate,
    run,
    deactivate /* or NULL */,
    cleanup,
    extension_data /* or NULL */
};

/* interface */
LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
  if (index == 0)
    return &descriptor;
  else
    return NULL;
}
