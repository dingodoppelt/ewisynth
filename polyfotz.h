#include <array>
#include <cstddef>
#include <math.h>
#include <cstdint>
#include <vector>
class PolyFotz {
private:
    float pitchbend = 1.f;
    float normalizedPitchbend = 0.f;
    float tune = 1.f;
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
        float   noteToFreq() { return powf(2.f, (getEffectiveNote() - 69.f) / 12.f) * 440.f; }
    } masterNote;
    uint8_t mode = 0;
    uint8_t activeVoicing = 0;
    uint8_t activeBank = 0;
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
    void setBank(uint8_t b) { if (activeBank != b) activeBank = b % banks[activeBank].size(); }
    void setVoicing(uint8_t v) { if (mode == 0 && activeVoicing != v) activeVoicing = v % banks[activeBank].size(); }
    void setRotator(uint8_t r) { if (mode != r) mode = r; }
    void setDetune(float d) { if (detune != d) detune = d; updateDetune(); }
    void setPolyphony(int8_t p) { if (polyphony != p) polyphony = p; updateDetune(); }
    float getFrequency(uint8_t voice) {
        float modulation = 1.f;
        if (pitchbend < 1.f) {
            if (voice < banks[activeBank][activeVoicing].size()) {
                modulation = pow(2., -normalizedPitchbend * banks[activeBank][activeVoicing][voice % banks[activeBank][activeVoicing].size()] / 12.);
            } else {
                modulation = detuneTable[voice];
            }
        } else {
            modulation = detuneTable[voice] * pitchbend;
        }
        return getMasterFrequency() * modulation;
    }
    void updateRotator() {
        switch (mode) {
            case 1:
                activeVoicing = (1 + activeVoicing) % banks[activeBank].size();
                break;
            case 2:
                activeVoicing = rand() % banks[activeBank].size();
                break;
            default:
                break;
        }
    }
    void Init(uint8_t maxPoly) { maxPolyphony = maxPoly; updateDetune(); }
private:
    const std::vector<std::vector<std::vector<int8_t>>> banks = {
        {{0, -7, -8, -14, -38}, // nb wide
        {0, -7, -8, -17, -39},
        {0, -7, -11, -18, -40},
        {0, -7, -9, -14, -38},
        {0, -7, -9, -17, -39},
        {0, -7, -10, -17, -39}},

        {{0, -7, -11, -18}, // blake
        {0, -7, -8, -15},
        {0, -5, -16, -23},
        {0, -5, -12, -20},
        {0, -7, -16, -23},
        {0, -7, -10, -29},
        {0, -7, -11, -18},
        {0, -5, -12, -20}},

        {{0, -7, -8, -14}, // nb
        {0, -7, -8, -17},
        {0, -7, -11, -18},
        {0, -7, -9, -14},
        {0, -7, -9, -17},
        {0, -7, -10, -17}},

        {{0, 7, -10}, // brecker
        {0, 7, -7},
        {0, 7, -8},
        {0, 7, -2}},

        {{0, -5, -7, -8, -15}, // ferrante
        {0, -5, -7, -9, -16}},

        {{0, -5, -10, -12}, // generic
        {0, -5, -10, -20},
        {0, -3, -8, -19},
        {0, -3, -7, -10},
        {0, -4, -9, -11},
        {0, -3, -8, -11},
        {0, -5, -7, -11},
        {0, -5, -11, -15},
        {0, -3, -6, -9},
        {0, -4, -8, -12},}
    };
};
