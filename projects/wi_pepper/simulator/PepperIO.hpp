#pragma once
#include <array>

// Future host I/O boundary. Ports are intentionally disconnected: no audio
// device, file, browser microphone, CV mapping or hardware backend is opened.
struct PepperIO {
    static constexpr unsigned int audioChannels = 2, cvChannels = 8;
    std::array<float, audioChannels> audioIn{}, audioOut{};
    std::array<float, cvChannels> cvIn{}, cvOut{};
    bool audioConnected = false, cvConnected = false;
};
