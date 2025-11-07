#include <cmath>    // For std::sin, M_PI (or std::numbers::pi in C++20)

// Define M_PI if not available (e.g., on some Windows compilers)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct FilterOutputs {
    double lowPass;
    double bandPass;
    double highPass;
    double dry;
};

class StateVariableFilter {
public:
    StateVariableFilter(double sampleRate, double centerFreq, double qFactor)
        : fs(sampleRate), f0(centerFreq), Q(qFactor), s1(0.0), s2(0.0) {
        calculateCoefficients();
    }

    FilterOutputs processSample(double input) {
        double hp = input - s2 - (q * s1);
        s1 = s1 + (f * hp);
        s2 = s2 + (f * s1);
        return {s2, s1, hp, input};
    }

    // Update filter parameters dynamically
    void setParams(double newF0, double newQ) {
        f0 = newF0;
        Q = newQ;
        calculateCoefficients();
    }

    // Get current parameters (optional, for verification)
    double getF0() const { return f0; }
    double getQ() const { return Q; }

private:
    double fs;
    double f0; // Center/cutoff frequency
    double Q;

    double s1;
    double s2;

    double f;
    double q;

    void calculateCoefficients() {
        if (f0 <= 0) f0 = 1e-6;
        if (f0 >= fs / 2) f0 = fs / 2 - 1e-6;
        if (Q <= 0) Q = 0.1;
        f = 2.0 * std::sin(M_PI * f0 / fs);
        q = 1.0 / Q;
    }
};