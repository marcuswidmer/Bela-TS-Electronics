#pragma once

// Keep the selected program LED lit until another program is selected.
class ProgramIndicator {
public:
    void setup(float /*sampleRate*/) { selected_ = false; }
    void select(unsigned int program) {
        program_ = program;
        selected_ = true;
    }
    bool active() const { return selected_; }
    bool ledOn(unsigned int led) const {
        return selected_ && led == program_;
    }
    void advance() {}
private:
    unsigned int program_ = 0;
    bool selected_ = false;
};
