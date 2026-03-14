#define MAX_RMS_BUFFER 2048

class EnvelopeFollower {
private:
    // Window length meaning number of samples to average over
    uint16_t rms_len;
    
    // precomputed scale factor depending on rms_len
    float invRms_len;
    
    // very simple circular buffer to store squared samples
    uint16_t    index;
    float       in_squared[MAX_RMS_BUFFER];
    
    // RMS estimated squared
    float       out_squared;


public:

    EnvelopeFollower() {
        init(MAX_RMS_BUFFER);
    }
    
    void init(uint16_t L) {
        rms_len = L;
        invRms_len = 1.f / (float)L;
        index = 0;
        std::fill(std::begin(in_squared), std::end(in_squared), 0);
        out_squared = 0.f;
    }

    float update(float in) {
        // square input sample and store in rms buffer
        float in_sq = in * in;
        in_squared[index] = in_sq;

        // increment index and wrap buffer index when needed
        if (++index == rms_len) index = 0;

        // actual RMS implementation
        out_squared = out_squared + invRms_len * (in_sq - in_squared[index]);
        return out_squared;
    }
};
