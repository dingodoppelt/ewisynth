#include <math.h>
#include <cstdint>
class PolyFotz {
private:
    float pitchbend = 1.f; // -1 - 1
    float tune = 1.f;    // -0.2 - 0.2
    uint8_t maxPolyphony = 16;
    uint8_t polyphony = 4;
    struct Note {
        uint8_t note = 69;
        uint8_t semitones = 0;
        uint8_t octaves = 0;
        uint8_t voicingOffset = 0;
        uint8_t getEffectiveNote() { return note + semitones + octaves + voicingOffset; }
        float   noteToFreq() { return powf(2.f, (getEffectiveNote() - 69.f) / 12.f) * 440.f ;}
    };
    Note masterNote;
    void setPolyphony(int p) { polyphony = (0 < p && p <= maxPolyphony) ? p : maxPolyphony; }
public:
    void setNote (uint8_t n) { masterNote.note = (0 <= n && n < 128) ? n : 69; }
    void setTranspose(uint8_t t) { masterNote.semitones = t; }
    void setOctave(uint8_t o) { masterNote.octaves = o * 12; }
    void setPitchbend(uint16_t b) { pitchbend = (float)pow(2., ((double)b - 8192.) / 49152.); }
    void setTune(float t) { tune = pow(2.0, t); }
    int getPolyphony() { return polyphony; }
    float getEffectiveFrequency() { return masterNote.noteToFreq() * tune * pitchbend; }
    void Init(uint8_t maxPoly) { maxPolyphony = maxPoly; }
};
