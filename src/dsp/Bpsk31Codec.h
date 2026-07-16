#pragma once

#include <string>
#include <vector>

namespace psk::dsp {

struct Bpsk31Config {
    double sampleRate = 8000.0;
    double symbolRate = 31.25;
    double carrierHz = 1000.0;
    double amplitude = 0.65;
    int preambleSymbols = 64;
    int postambleSymbols = 64;

    // RX Costas (carrier) and Gardner (timing) loop filter gains. See the
    // validated-envelope comment in Bpsk31Codec.cpp for what these were
    // tuned and tested against. Exposed here (rather than hardcoded) so
    // they can be swept/tuned without editing the codec, and so a future
    // properly-designed loop filter (zeta/omega_n mapping) can replace
    // these empirical defaults without an API change.
    double costasKp = 0.06;
    double costasKi = 0.004;
    double gardnerKp = 1.50;
    double gardnerKi = 0.01875;
};

class Bpsk31Codec {
public:
    explicit Bpsk31Codec(Bpsk31Config config = {});

    std::vector<int> frameTextBits(const std::string &text) const;
    std::vector<double> modulateText(const std::string &text) const;
    std::vector<int> demodulateBits(const std::vector<double> &samples) const;
    std::string demodulateText(const std::vector<double> &samples) const;

    int samplesPerSymbol() const;
    Bpsk31Config config() const;

private:
    Bpsk31Config m_config;
    std::vector<int> trackWithOffset(const std::vector<double> &samples, double offsetHz) const;
    double scoreDecodedBits(const std::vector<int> &bits) const;
};

} // namespace psk::dsp
