// ported from https://github.com/BramGiesen/audio-to-cv-pitch-lv2 in 2026 by Nils Brederlow

extern "C" {
    #include <aubio.h>
}

class PitchTracker {
    
private:
    aubio_pitch_t* pitchDetector;
    fvec_t* const detectedPitch;
    
    fvec_t* inputBuffer;
    uint32_t inputBufferPos;
    uint32_t inputBufferSize;
    
    float lastKnownPitchInHz;
    float lastKnownPitchConfidence;

    bool  holdOutputPitch;
    
    
    void recreateAubioPitchDetector(double sampleRate) {
        float tolerance;
        
        if (pitchDetector != nullptr)
        {
            tolerance = aubio_pitch_get_tolerance(pitchDetector);
            del_aubio_pitch(pitchDetector);
        }
        else
        {
            tolerance = 0.625f;
        }
        
        pitchDetector = new_aubio_pitch("yinfast", inputBufferSize, inputBufferSize / 4, sampleRate);
        DISTRHO_SAFE_ASSERT_RETURN(pitchDetector != nullptr,);
        
        aubio_pitch_set_silence(pitchDetector, -30.0f);
        aubio_pitch_set_tolerance(pitchDetector, tolerance);
        aubio_pitch_set_unit(pitchDetector, "Hz");
    }
    
public:
    float threshold;
    float sensitivity;
    PitchTracker(double sr, uint32_t bufsize)
    : detectedPitch(new_fvec(1)),
      pitchDetector(nullptr)
    {
        inputBufferSize = bufsize;
        recreateAubioPitchDetector(sr);
        
        lastKnownPitchInHz = 0.0f;
        lastKnownPitchConfidence = 0.0f;
        
        sensitivity = 60.0f;
        threshold = 80.f;
        holdOutputPitch = true;
        
        inputBuffer = new_fvec(inputBufferSize);
        inputBufferPos = 0;
    }
    ~PitchTracker() {
        if (pitchDetector != nullptr)
            del_aubio_pitch(pitchDetector);
        
        del_fvec(inputBuffer);
        del_fvec(detectedPitch);
        aubio_cleanup();
    }

    void processBlock(const float** input, float* output, uint32_t numFrames)
    {
        if (d_isEqual(sensitivity, 1.0f))
        {
            std::memcpy(inputBuffer->data + inputBufferPos, input[0], sizeof(float)*numFrames);
        }
        else
        {
            // TODO replace with faster SSE/NEON multiply and assign
            for (uint32_t i = 0; i < numFrames; ++i)
                inputBuffer->data[inputBufferPos + i] = input[0][i] * sensitivity;
        }
        
        inputBufferPos += numFrames;
        
        float cvPitch, cvSignal;
        
        if (inputBufferPos >= inputBufferSize)
        {
            inputBufferPos -= inputBufferSize;
            
            aubio_pitch_do(pitchDetector, inputBuffer, detectedPitch);
            const float detectedPitchInHz = fvec_get_sample(detectedPitch, 0);
            const float pitchConfidence = aubio_pitch_get_confidence(pitchDetector) * 100;
            
            if (detectedPitchInHz > 0.f && pitchConfidence >= threshold)
            {
                cvPitch = detectedPitchInHz;
                lastKnownPitchInHz = detectedPitchInHz;
                cvSignal = 1.f;
            }
            else if (holdOutputPitch)
            {
                cvPitch = lastKnownPitchInHz;
                cvSignal = 0.f;
            }
            else
            {
                lastKnownPitchInHz = cvPitch = 0.0f;
                cvSignal = 0.f;
            }
            
            lastKnownPitchConfidence = pitchConfidence;
        }
        else
        {
            cvPitch = lastKnownPitchInHz;
            cvSignal = cvPitch > 0.0f ? 1.0f : 0.0f;
        }
        output[0] = cvPitch;
        output[1] = cvSignal;
    }
};
