#include <math.h>

class Curves {
private:
    float x_ = 1.f;
    float y_ = 0.f;
    float exponent = 1.f;
public:
    Curves() : x_(1.f), y_(0.f) {}

    float apply(float t) {
        if (0.f <= t <= 1.f) {
            float texp = pow(t, exponent);
            float Ay = t * y_;
            float By = (1-y_) * texp + y_;
            return (By - Ay) * t + Ay;
        }
    }

    void setX(float new_x) {
        x_ = new_x;
    }

    void setY(float new_y) {
        y_ = new_y;
    }
};
