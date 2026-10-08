#pragma once
#include <array>

// Host I/O boundary. The simulator exports audioOut for browser playback;
// audioIn stays silent and CV ports remain disconnected. audioConnected
// indicates that the host supports output transport, not microphone capture.
struct PepperIO {
    static constexpr unsigned int audioChannels = 2, cvChannels = 8;
    std::array<float, audioChannels> audioIn{}, audioOut{};
    std::array<float, cvChannels> cvIn{}, cvOut{};
    bool audioConnected = false, cvConnected = false;
};
