{
/**         
       @brief Variable Waveshape Oscillator. 
       @author Ben Sergentanis
       @date Dec 2020 
       Continuously variable waveform. \n \n
       Ported from pichenettes/eurorack/plaits/dsp/oscillator/variable_shape_oscillator.h \n 
       \n to an independent module. \n
       Original code written by Emilie Gillet in 2016. \n
       
       made into a single file and slimmed down by Nils Brederlow in 2023
*/

class VariableShapeOscillator
{
public:
    VariableShapeOscillator() {}
    ~VariableShapeOscillator() {}

    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;

        slave_phase_  = 0.0f;
        next_sample_  = 0.0f;
        previous_pw_  = 0.5f;
        high_         = false;

        SetFreq(440.f);
        SetWaveshape(0.f);
        SetPW(.5f);
    }

    float Process()
    {
        float next_sample = next_sample_;

        float this_sample = next_sample;
        next_sample       = 0.0f;

        const float square_amount   = fmax(waveshape_ - 0.5f, 0.0f) * 2.0f;
        const float triangle_amount = fmax(1.0f - waveshape_ * 2.0f, 0.0f);
        const float slope_up        = 1.0f / (pw_);
        const float slope_down      = 1.0f / (1.0f - pw_);

        slave_phase_ += slave_frequency_;
        if(!high_)
        {
            if(slave_phase_ > pw_)
            {
                float t = (slave_phase_ - pw_)
                / (previous_pw_ - pw_ + slave_frequency_);
                float triangle_step = (slope_up + slope_down) * slave_frequency_;
                triangle_step *= triangle_amount;

                this_sample += square_amount * ThisBlepSample(t);
                next_sample += square_amount * NextBlepSample(t);
                this_sample -= triangle_step * ThisIntegratedBlepSample(t);
                next_sample -= triangle_step * NextIntegratedBlepSample(t);
                high_ = true;
            }
        }

        if(high_)
        {
            if(slave_phase_ > 1.0f)
            {
                slave_phase_ -= 1.0f;
                float t             = slave_phase_ / slave_frequency_;
                float triangle_step = (slope_up + slope_down) * slave_frequency_;
                triangle_step *= triangle_amount;

                this_sample -= (1.0f - triangle_amount) * ThisBlepSample(t);
                next_sample -= (1.0f - triangle_amount) * NextBlepSample(t);
                this_sample += triangle_step * ThisIntegratedBlepSample(t);
                next_sample += triangle_step * NextIntegratedBlepSample(t);
                high_ = false;
            }
        }

        next_sample += ComputeNaiveSample(slave_phase_,
                                          pw_,
                                          slope_up,
                                          slope_down,
                                          triangle_amount,
                                          square_amount);
        previous_pw_ = pw_;


        next_sample_ = next_sample;
        return (2.0f * this_sample - 1.0f);
    }

    void SetFreq(float frequency)
    {
        frequency         = frequency / sample_rate_;
        frequency         = frequency >= .25f ? .25f : frequency;
        slave_frequency_ = frequency;
    }

    void SetPW(float pw)
    {
        if(slave_frequency_ >= .25f)
        {
            pw_ = .5f;
        }
        else
        {
            pw_ = fclamp(
                pw, slave_frequency_ * .5f, 1.0f - .5f * slave_frequency_);
        }
    }

    void SetWaveshape(float waveshape)
    {
        waveshape_ = waveshape;
    }

    void OffsetPhase(float offset)
    {
        slave_phase_ += offset;
    }

private:
    float sample_rate_;

    // Oscillator state.
    float slave_phase_;
    float next_sample_;
    float previous_pw_;
    bool  high_;

    // For interpolation of parameters.
    float slave_frequency_;
    float pw_;
    float waveshape_;
    float ComputeNaiveSample(float phase,
                             float pw,
                             float slope_up,
                             float slope_down,
                             float triangle_amount,
                             float square_amount)
    {
        float saw    = phase;
        float square = phase < pw ? 0.0f : 1.0f;
        float triangle
        = phase < pw ? phase * slope_up : 1.0f - (phase - pw) * slope_down;
        saw += (square - saw) * square_amount;
        saw += (triangle - saw) * triangle_amount;
        return saw;
    }
    float fmax(float a, float b)
    {
        float r;
        r = (a > b) ? a : b;
        return r;
    }

    float fmin(float a, float b)
    {
        float r;
        r = (a < b) ? a : b;
        return r;
    }

    /** quick fp clamp
     */
    float fclamp(float in, float min, float max)
    {
        return fmin(fmax(in, min), max);
    }

    /** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
     */
    float ThisBlepSample(float t)
    {
        return 0.5f * t * t;
    }

    /** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
     */
    float NextBlepSample(float t)
    {
        t = 1.0f - t;
        return -0.5f * t * t;
    }

    /** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
     */
    float NextIntegratedBlepSample(float t)
    {
        const float t1 = 0.5f * t;
        const float t2 = t1 * t1;
        const float t4 = t2 * t2;
        return 0.1875f - t1 + 1.5f * t2 - t4;
    }

    /** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
     */
    float ThisIntegratedBlepSample(float t)
    {
        return NextIntegratedBlepSample(1.0f - t);
    }
};
