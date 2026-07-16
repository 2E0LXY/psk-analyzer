#include "ConvCode.h"

#include <limits>
#include <utility>

namespace psk::dsp {
namespace {

int parity(unsigned v)
{
    int p = 0;
    while (v) {
        p ^= (v & 1u);
        v >>= 1;
    }
    return p;
}

} // namespace

std::vector<int> ConvCode::encode(const std::vector<int> &bits)
{
    std::vector<int> out;
    out.reserve((bits.size() + kConstraintLength - 1) * 2);
    unsigned reg = 0;
    const unsigned mask = (1u << kConstraintLength) - 1u;

    auto step = [&](int bit) {
        reg = ((reg << 1) | static_cast<unsigned>(bit & 1)) & mask;
        out.push_back(parity(reg & kGenerator1));
        out.push_back(parity(reg & kGenerator2));
    };

    for (int b : bits) {
        step(b);
    }
    // Flush to the all-zero state so the decoder has a known termination
    // point - standard convolutional code framing.
    for (int i = 0; i < kConstraintLength - 1; ++i) {
        step(0);
    }

    return out;
}

std::vector<int> ConvCode::decodeSoft(const std::vector<double> &softBits, int messageBitCount)
{
    const int steps = static_cast<int>(softBits.size() / 2);
    const double kNegInf = -std::numeric_limits<double>::infinity();
    const unsigned mask = (1u << kConstraintLength) - 1u;
    const unsigned stateMask = (1u << (kConstraintLength - 1)) - 1u;

    std::vector<double> pathMetric(kNumStates, kNegInf);
    std::vector<double> nextPathMetric(kNumStates, kNegInf);
    pathMetric[0] = 0.0; // encoder always starts in the all-zero state

    // traceback[t][state] = {previousState, inputBitThatArrivedHere}
    std::vector<std::vector<std::pair<int, int>>> traceback(
        static_cast<std::size_t>(steps), std::vector<std::pair<int, int>>(kNumStates, {-1, 0}));

    for (int t = 0; t < steps; ++t) {
        std::fill(nextPathMetric.begin(), nextPathMetric.end(), kNegInf);
        const double s1 = softBits[static_cast<std::size_t>(2 * t)];
        const double s2 = softBits[static_cast<std::size_t>(2 * t + 1)];

        for (int state = 0; state < kNumStates; ++state) {
            if (pathMetric[static_cast<std::size_t>(state)] == kNegInf) {
                continue;
            }
            for (int bit = 0; bit <= 1; ++bit) {
                const unsigned reg = ((static_cast<unsigned>(state) << 1) | static_cast<unsigned>(bit)) & mask;
                const int expected1 = parity(reg & kGenerator1);
                const int expected2 = parity(reg & kGenerator2);
                // Soft correlation metric: agreement between the expected
                // (encoder-predicted) bit and the received soft value adds
                // to the path metric; Viterbi keeps the maximum-likelihood
                // (here, maximum-correlation) path into each state.
                const double branchMetric =
                    s1 * (expected1 ? 1.0 : -1.0) + s2 * (expected2 ? 1.0 : -1.0);
                const unsigned nextState = reg & stateMask;
                const double metric = pathMetric[static_cast<std::size_t>(state)] + branchMetric;
                if (metric > nextPathMetric[nextState]) {
                    nextPathMetric[nextState] = metric;
                    traceback[static_cast<std::size_t>(t)][nextState] = {state, bit};
                }
            }
        }
        pathMetric.swap(nextPathMetric);
    }

    // The encoder is flushed to state 0 at the end, so that's the correct
    // path to trace back from for a well-formed buffer. Falling back to
    // the overall best state keeps this robust against a truncated buffer
    // (e.g. a live rolling audio window) rather than failing outright.
    int bestState = 0;
    double bestMetric = pathMetric[0];
    for (int s = 1; s < kNumStates; ++s) {
        if (pathMetric[static_cast<std::size_t>(s)] > bestMetric) {
            bestMetric = pathMetric[static_cast<std::size_t>(s)];
            bestState = s;
        }
    }

    std::vector<int> decodedBits(static_cast<std::size_t>(steps));
    int state = bestState;
    for (int t = steps - 1; t >= 0; --t) {
        const auto [prevState, bit] = traceback[static_cast<std::size_t>(t)][static_cast<std::size_t>(state)];
        decodedBits[static_cast<std::size_t>(t)] = bit;
        state = prevState;
    }

    if (static_cast<int>(decodedBits.size()) > messageBitCount) {
        decodedBits.resize(static_cast<std::size_t>(messageBitCount));
    }
    return decodedBits;
}

} // namespace psk::dsp
