#include <array>
#include <cstddef>
#include <math.h>
#include <cstdint>
class PolyFotz {
private:
    float pitchbend = 1.f; // -1 - 1
    float normalizedPitchbend = 0.f;
    float tune = 1.f;    // -0.2 - 0.2
    float detune = 0.f;
    float phase = 0;
    uint8_t maxPolyphony = 16;
    uint8_t polyphony = 4;
    struct MasterNote {
        uint8_t note = 69;
        uint8_t semitones = 0;
        uint8_t octaves = 0;
        uint8_t voicingOffset = 0;
        uint8_t getEffectiveNote() { return note + semitones + octaves + voicingOffset; }
        float   noteToFreq() { return powf(2.f, (getEffectiveNote() - 69.f) / 12.f) * 440.f ;}
    };
    // template<typename T, size_t N >
    // struct Voicing {
    //     std::array<T, N> voices;
    //     uint8_t length = N;
    //     float toFactor(uint8_t v) { return pow(2., voices[v % N] / 12.); }
    //     Voicing(const std::array<T, N>& _voices) : voices(_voices) {}
    // };
    struct Voicing {
        int8_t voices[4] = { 0, -5, -10, -12};
        uint8_t length = 4;
        float toFactor(uint8_t v, float coef) { return pow(2., coef * voices[v % length] / 12.); }
    };
    MasterNote masterNote;
    Voicing voicing;
    uint8_t activeBank = 0;
    uint8_t activeVoicing = 0;
    bool rotator = 0;
    float detuneTable[16] = {1};
    float getMasterFrequency() { return masterNote.noteToFreq() * tune; }
    void updateDetune() {
        for (int i = 0; i < polyphony; i++) {
            detuneTable[i] = pow(2., ((i * detune) / (polyphony << 6)) * cos(M_PI * i));
        }
    }
public:
    void setNote (uint8_t n) { masterNote.note = (0 <= n && n < 128) ? n : 69; }
    void setTranspose(uint8_t t) { masterNote.semitones = t; }
    void setOctave(uint8_t o) { masterNote.octaves = o * 12; }
    void setPitchbend(uint16_t b) { normalizedPitchbend = ((double)b - 8192.) / 8192.; pitchbend = pow(2., ((double)b - 8192.) / 49152.); }
    void setTune(float t) { tune = pow(2.0, t); }
    void setBank(uint8_t b) { if (activeBank != b) activeBank = b; }
    void setVoicing(uint8_t v) { if (activeVoicing != v) activeVoicing = v; }
    void setRotator(uint8_t r) { if (rotator != r) rotator = r; }
    void setDetune(float d) { if (detune != d) detune = d; updateDetune(); }
    void setPolyphony(int8_t p) { if (polyphony != p) polyphony = p; updateDetune(); }
    float getFrequency(uint8_t voice) {
        float modulation = 1.f;
        if (pitchbend < 1.f && voice < voicing.length) {
            modulation = voicing.toFactor(voice, -normalizedPitchbend ) ;
        } else {
            modulation = detuneTable[voice] * pitchbend;
        }
        return getMasterFrequency() * modulation; }
    void Init(uint8_t maxPoly) { maxPolyphony = maxPoly; updateDetune(); }
};
