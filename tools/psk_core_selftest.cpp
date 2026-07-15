#include "dsp/Bpsk31Codec.h"
#include "dsp/PskVaricode.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace {

// Golden vectors from the ARRL/G3PLX PSK31 Varicode specification
// (https://www.arrl.org/psk31-spec). A pure round-trip test cannot catch a
// wrong table entry because encode and decode share the same table; these
// vectors are transcribed independently to catch that class of bug.
bool checkGoldenVaricodeVectors(std::string *error)
{
    static const std::unordered_map<unsigned char, std::string> golden = {
        {' ', "1"}, {'e', "11"}, {'t', "101"}, {'a', "1011"},
        {'C', "10101101"}, {'Q', "1111011101"}, {'K', "101111101"},
        {'0', "10110111"}, {'9', "110110111"},
    };

    for (const auto &[ch, expected] : golden) {
        const std::string actual = psk::dsp::PskVaricode::codeForAscii(ch);
        if (actual != expected) {
            if (error) {
                *error = std::string("Varicode mismatch for '") + static_cast<char>(ch)
                    + "': expected " + expected + " got " + actual;
            }
            return false;
        }
    }
    return true;
}

// Simulates a TX/RX sample-clock mismatch (independent soundcard clocks),
// which shows up to the demodulator as gradual symbol timing drift.
std::vector<double> resample(const std::vector<double> &in, double rateFactor)
{
    std::vector<double> out;
    const std::size_t outLen = static_cast<std::size_t>(in.size() / rateFactor);
    out.reserve(outLen);
    for (std::size_t i = 0; i < outLen; ++i) {
        const double srcPos = static_cast<double>(i) * rateFactor;
        const auto i0 = static_cast<std::size_t>(srcPos);
        if (i0 + 1 >= in.size()) {
            break;
        }
        const double frac = srcPos - static_cast<double>(i0);
        out.push_back(in[i0] * (1.0 - frac) + in[i0 + 1] * frac);
    }
    return out;
}

// Regression fixtures for the Costas carrier PLL + Gardner symbol-timing
// loop in Bpsk31Codec::demodulateBits. Validated envelope: carrier offset
// up to +/-7Hz, sample-clock drift up to +/-0.1% - see the gain comment in
// Bpsk31Codec.cpp for why 7-8Hz is a real architectural ceiling for a
// symbol-rate Costas loop on differentially-encoded BPSK, not a bug.
bool checkImpairmentRecovery(std::string *error)
{
    const std::string message = "CQ CQ DE 2E0LXY 2E0LXY K THE QUICK BROWN FOX 73";

    struct FreqCase { double offsetHz; };
    for (const FreqCase c : {FreqCase{2.0}, FreqCase{5.0}, FreqCase{7.0}, FreqCase{-7.0}}) {
        psk::dsp::Bpsk31Config txConfig;
        txConfig.carrierHz = 1000.0 + c.offsetHz;
        const psk::dsp::Bpsk31Codec txCodec(txConfig);
        const std::vector<double> samples = txCodec.modulateText(message);

        const psk::dsp::Bpsk31Codec rxCodec; // nominal 1000Hz
        const std::string decoded = rxCodec.demodulateText(samples);
        if (decoded.find(message) == std::string::npos) {
            if (error) {
                *error = "Costas loop failed to recover " + std::to_string(c.offsetHz)
                    + "Hz carrier offset (within the validated +/-7Hz envelope)";
            }
            return false;
        }
    }

    struct DriftCase { double rateFactor; };
    for (const DriftCase c : {DriftCase{1.0005}, DriftCase{0.9995}, DriftCase{1.001}, DriftCase{0.999}}) {
        const psk::dsp::Bpsk31Codec txCodec;
        const std::vector<double> samples = txCodec.modulateText(message);
        const std::vector<double> drifted = resample(samples, c.rateFactor);

        const psk::dsp::Bpsk31Codec rxCodec;
        const std::string decoded = rxCodec.demodulateText(drifted);
        if (decoded.find(message) == std::string::npos) {
            if (error) {
                *error = "Gardner loop failed to recover clock drift factor "
                    + std::to_string(c.rateFactor) + " (within the validated +/-0.1% envelope)";
            }
            return false;
        }
    }

    // Combined frequency offset + clock drift, both loops running at once.
    {
        psk::dsp::Bpsk31Config txConfig;
        txConfig.carrierHz = 1004.0;
        const psk::dsp::Bpsk31Codec txCodec(txConfig);
        const std::vector<double> samples = txCodec.modulateText(message);
        const std::vector<double> drifted = resample(samples, 1.0007);

        const psk::dsp::Bpsk31Codec rxCodec;
        const std::string decoded = rxCodec.demodulateText(drifted);
        if (decoded.find(message) == std::string::npos) {
            if (error) {
                *error = "Combined Costas+Gardner recovery failed for 4Hz offset + 0.07% drift";
            }
            return false;
        }
    }

    // Sanity check that the documented ~8Hz ceiling is real and hasn't
    // silently vanished or silently gotten worse: 10Hz is expected to
    // fail. If this starts passing, the envelope comment needs updating;
    // if something within the validated range starts failing near this
    // boundary, that's a regression.
    {
        psk::dsp::Bpsk31Config txConfig;
        txConfig.carrierHz = 1010.0;
        const psk::dsp::Bpsk31Codec txCodec(txConfig);
        const std::vector<double> samples = txCodec.modulateText(message);
        const psk::dsp::Bpsk31Codec rxCodec;
        const std::string decoded = rxCodec.demodulateText(samples);
        if (decoded.find(message) != std::string::npos) {
            if (error) {
                *error = "10Hz offset unexpectedly decoded - envelope comment in "
                    "Bpsk31Codec.cpp is now understating the loop's actual pull-in range";
            }
            return false;
        }
    }

    return true;
}

} // namespace

int main()
{
    std::string error;
    if (!psk::dsp::PskVaricode::validateTable(&error)) {
        std::cerr << "Varicode table invalid: " << error << '\n';
        return 1;
    }

    if (!checkGoldenVaricodeVectors(&error)) {
        std::cerr << "Varicode golden vector check failed: " << error << '\n';
        return 4;
    }

    std::string printable;
    for (char ch = 32; ch < 127; ++ch) {
        printable.push_back(ch);
    }

    const auto printableBits = psk::dsp::PskVaricode::encodeTextBits(printable);
    const std::string printableRoundtrip = psk::dsp::PskVaricode::decodeTextBits(printableBits);
    if (printableRoundtrip != printable) {
        std::cerr << "Varicode printable roundtrip failed\n";
        return 2;
    }

    const std::string message = "CQ CQ DE 2E0LXY 2E0LXY K";
    psk::dsp::Bpsk31Codec codec;
    const std::vector<double> samples = codec.modulateText(message);
    const std::string decoded = codec.demodulateText(samples);
    if (decoded.find(message) == std::string::npos) {
        std::cerr << "BPSK31 roundtrip failed\n";
        std::cerr << "Expected fragment: " << message << '\n';
        std::cerr << "Decoded: " << decoded << '\n';
        return 3;
    }

    if (!checkImpairmentRecovery(&error)) {
        std::cerr << "Impairment recovery check failed: " << error << '\n';
        return 5;
    }

    std::cout << "PSK core self-test passed\n";
    std::cout << "Decoded: " << decoded << '\n';
    return 0;
}
