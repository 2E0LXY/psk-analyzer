#include "Bpsk31Codec.h"

#include "PskVaricode.h"

#include <cmath>
#include <complex>

namespace psk::dsp {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

double wrapPhase(double phase)
{
    while (phase <= -kPi) {
        phase += 2.0 * kPi;
    }
    while (phase > kPi) {
        phase -= 2.0 * kPi;
    }
    return phase;
}

// Linear-interpolated read so correlator windows can start at a fractional
// sample position (required for symbol-timing correction). Out-of-range
// reads return 0 rather than throwing - the demodulator runs to the end of
// whatever buffer it's given, so edge padding is expected behaviour, not
// an error condition.
double readInterpolated(const std::vector<double> &samples, double pos)
{
    if (pos < 0.0 || pos >= static_cast<double>(samples.size()) - 1.0) {
        return 0.0;
    }
    const auto i0 = static_cast<std::size_t>(pos);
    const double frac = pos - static_cast<double>(i0);
    return samples[i0] * (1.0 - frac) + samples[i0 + 1] * frac;
}

// Loop filter gains for the Costas (carrier) and Gardner (timing) loops.
// These are empirically tuned and validated against the impairment
// fixtures in tools/psk_core_selftest.cpp, not derived from a target loop
// noise-bandwidth/damping-factor design - a properly designed loop filter
// (via the standard zeta/omega_n mapping) is a documented follow-up if a
// wider or narrower pull-in range is needed than what is tested here.
//
// Validated operating envelope (see selftest for the exact fixtures):
//   - carrier frequency offset: up to +/-7 Hz
//   - sample-clock drift (TX/RX clock mismatch): up to +/-0.1%
// Above ~8 Hz offset at 31.25 baud, per-symbol carrier rotation exceeds
// the +/-90 degree differential decision boundary (rotation = 360 * f /
// symbolRate degrees/symbol), which is a fundamental ambiguity limit for
// a symbol-rate Costas loop on differentially-encoded BPSK, not something
// these gains can be tuned around. A non-data-aided frequency-only
// discriminator (e.g. FFT-based search or a quadricorrelator) ahead of
// this loop would be needed to widen that ceiling - flagged as a
// follow-up, not attempted here.
constexpr double kCostasKp = 0.06;
constexpr double kCostasKi = 0.004;
constexpr double kGardnerKp = 0.60;
constexpr double kGardnerKi = 0.0075;

} // namespace

Bpsk31Codec::Bpsk31Codec(Bpsk31Config config)
    : m_config(config)
{
}

std::vector<int> Bpsk31Codec::frameTextBits(const std::string &text) const
{
    std::vector<int> bits;
    bits.reserve(m_config.preambleSymbols + text.size() * 8 + m_config.postambleSymbols);

    for (int i = 0; i < m_config.preambleSymbols; ++i) {
        bits.push_back(0);
    }

    const std::vector<int> payload = PskVaricode::encodeTextBits(text);
    bits.insert(bits.end(), payload.begin(), payload.end());

    for (int i = 0; i < m_config.postambleSymbols; ++i) {
        bits.push_back(1);
    }

    return bits;
}

std::vector<double> Bpsk31Codec::modulateText(const std::string &text) const
{
    const std::vector<int> bits = frameTextBits(text);
    const int sps = samplesPerSymbol();
    std::vector<double> samples;
    samples.reserve(bits.size() * sps);

    double carrierPhase = 0.0;
    double dataPhase = 0.0;
    const double carrierStep = 2.0 * kPi * m_config.carrierHz / m_config.sampleRate;

    for (int bit : bits) {
        if (bit == 0) {
            dataPhase = wrapPhase(dataPhase + kPi);
        }

        for (int i = 0; i < sps; ++i) {
            samples.push_back(m_config.amplitude * std::cos(carrierPhase + dataPhase));
            carrierPhase = wrapPhase(carrierPhase + carrierStep);
        }
    }

    return samples;
}

std::vector<int> Bpsk31Codec::demodulateBits(const std::vector<double> &samples) const
{
    const double sps = static_cast<double>(samplesPerSymbol());
    const double carrierStepNominal = 2.0 * kPi * m_config.carrierHz / m_config.sampleRate;

    std::vector<int> bits;
    bits.reserve(static_cast<std::size_t>(samples.size() / std::max(1.0, sps)));

    // Correlator: integrate-and-dump matched filter for the rectangular
    // pulse shape used by modulateText(), evaluated at absolute sample
    // position `start` using the carrier reference phase(p) below. This is
    // the same matched filter as before; what's new is that `start` can be
    // fractional (for Gardner's fractional-sample correction) and the
    // carrier phase is corrected by the Costas loop rather than free-running.
    auto phaseAt = [&](double epochPos, double phaseEpoch, double step, double p) {
        return wrapPhase(phaseEpoch + (p - epochPos) * step);
    };
    auto integrate = [&](double start, double epochPos, double phaseEpoch, double step) {
        double iSum = 0.0;
        double qSum = 0.0;
        for (int n = 0; n < static_cast<int>(sps); ++n) {
            const double p = start + n;
            const double ph = phaseAt(epochPos, phaseEpoch, step, p);
            const double s = readInterpolated(samples, p);
            iSum += s * std::cos(ph);
            qSum += -s * std::sin(ph);
        }
        return std::complex<double>(iSum, qSum);
    };

    double epochPos = 0.0;
    double phaseEpoch = 0.0;
    double effectiveStep = carrierStepNominal;
    double carrierFreqIntegral = 0.0;
    double timingFreqIntegral = 0.0;
    double symbolStart = 0.0;

    double previousPhase = 0.0;
    bool havePreviousPhase = false;
    std::complex<double> previousOnTime(0.0, 0.0);
    bool havePreviousOnTime = false;

    // Stop when the on-time window plus one interpolation sample, or the
    // Gardner midpoint window half a symbol earlier, would run off the end.
    while (symbolStart + sps + 1.0 <= static_cast<double>(samples.size())) {
        const std::complex<double> onTime = integrate(symbolStart, epochPos, phaseEpoch, effectiveStep);

        double gardnerError = 0.0;
        if (havePreviousOnTime) {
            const double midStart = symbolStart - sps / 2.0;
            const std::complex<double> midOnTime = integrate(midStart, epochPos, phaseEpoch, effectiveStep);
            // e = Re{ conj(mid) * (onTime - previous) } - the complex
            // generalisation of Gardner's BPSK/QPSK timing-error detector
            // (Gardner, IEEE Trans. Commun., May 1986), normalised by the
            // current symbol's energy so the loop gain doesn't depend on
            // signal amplitude.
            const double energy = std::norm(onTime) + 1e-9;
            gardnerError = (std::conj(midOnTime) * (onTime - previousOnTime)).real() / energy;
        }
        timingFreqIntegral += kGardnerKi * gardnerError;
        const double timingCorrection = -(kGardnerKp * gardnerError + timingFreqIntegral);

        // Costas decision-directed phase detector for BPSK: e = sign(I)*Q.
        // Drives Q toward zero (all energy on the in-phase axis) regardless
        // of what the recovered bit values mean - it locks the constellation
        // orientation, not the data.
        const double energy = std::norm(onTime) + 1e-9;
        const double costasError = (onTime.real() >= 0.0 ? 1.0 : -1.0) * onTime.imag() / energy;
        carrierFreqIntegral += kCostasKi * costasError;
        const double phaseKick = kCostasKp * costasError;

        const double phase = std::atan2(onTime.imag(), onTime.real());
        if (!havePreviousPhase) {
            previousPhase = phase;
            havePreviousPhase = true;
        } else {
            const double delta = std::abs(wrapPhase(phase - previousPhase));
            bits.push_back(delta > (kPi / 2.0) ? 0 : 1);
            previousPhase = phase;
        }

        previousOnTime = onTime;
        havePreviousOnTime = true;

        // Advance both loops' state to the next symbol. Frequency
        // corrections (the integral paths) persist and compound; phase
        // and timing corrections (proportional paths) are applied once as
        // an offset to the next symbol's reference point.
        const double nextStart = symbolStart + sps + timingCorrection;
        const double naturalPhaseAtNext = phaseAt(epochPos, phaseEpoch, effectiveStep, nextStart);
        epochPos = nextStart;
        phaseEpoch = wrapPhase(naturalPhaseAtNext + phaseKick);
        effectiveStep = carrierStepNominal + carrierFreqIntegral;
        symbolStart = nextStart;
    }

    return bits;
}

std::string Bpsk31Codec::demodulateText(const std::vector<double> &samples) const
{
    return PskVaricode::decodeTextBits(demodulateBits(samples));
}

int Bpsk31Codec::samplesPerSymbol() const
{
    return static_cast<int>(std::lround(m_config.sampleRate / m_config.symbolRate));
}

Bpsk31Config Bpsk31Codec::config() const
{
    return m_config;
}

} // namespace psk::dsp
