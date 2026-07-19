#include "Ldpc174_91.h"
#include "Ldpc174_91_data.h"

#include <cmath>

namespace psk::dsp::ft8 {

std::vector<int> Ldpc174_91::encode(const std::vector<int> &messageBits)
{
    std::vector<int> codeword(174, 0);
    // Systematic code: the first 91 codeword bits are the message itself,
    // unchanged.
    for (int i = 0; i < 91; ++i) {
        codeword[static_cast<std::size_t>(i)] = messageBits[static_cast<std::size_t>(i)];
    }
    // Parity bit i = XOR (mod-2 sum) of message bits where kGenerator[i] has a 1.
    for (int row = 0; row < 83; ++row) {
        int parity = 0;
        const auto &genRow = kGenerator[static_cast<std::size_t>(row)];
        for (int col = 0; col < 91; ++col) {
            if (genRow[static_cast<std::size_t>(col)] != 0
                && messageBits[static_cast<std::size_t>(col)] != 0) {
                parity ^= 1;
            }
        }
        codeword[static_cast<std::size_t>(91 + row)] = parity;
    }
    return codeword;
}

bool Ldpc174_91::checkParity(const std::vector<int> &codeword)
{
    for (const auto &check : kNm) {
        int x = 0;
        for (int idx : check) {
            if (idx != 0) {
                x ^= codeword[static_cast<std::size_t>(idx - 1)];
            }
        }
        if (x != 0) {
            return false;
        }
    }
    return true;
}

std::optional<std::vector<int>> Ldpc174_91::decode(const std::vector<double> &softBits,
                                                     int maxIterations)
{
    if (softBits.size() != 174) {
        return std::nullopt;
    }

    // Sum-product (belief propagation) decoding - mirrors the reference
    // Python implementation's structure (see Ldpc174_91_data.h's comment
    // for provenance) exactly, translated to C++, not a different
    // algorithm that happens to also decode LDPC codes: m[j][i] is the
    // message from check j to bit i (codeword[i]'s log-likelihood based
    // on information other than check j); e[j][i] is the message from
    // check j back to bit i, computed from the OTHER bits check j
    // references.
    std::array<std::array<double, 174>, 83> m{};
    for (int j = 0; j < 83; ++j) {
        for (int i = 0; i < 174; ++i) {
            m[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = softBits[static_cast<std::size_t>(i)];
        }
    }

    for (int iter = 0; iter < maxIterations; ++iter) {
        std::array<std::array<double, 174>, 83> e{};
        for (int j = 0; j < 83; ++j) {
            const auto &check = kNm[static_cast<std::size_t>(j)];
            for (int slot = 0; slot < 7; ++slot) {
                const int i = check[static_cast<std::size_t>(slot)];
                if (i <= 0) {
                    continue;
                }
                double a = 1.0;
                for (int otherSlot = 0; otherSlot < 7; ++otherSlot) {
                    if (otherSlot == slot) {
                        continue;
                    }
                    const int ii = check[static_cast<std::size_t>(otherSlot)];
                    if (ii <= 0) {
                        continue;
                    }
                    a *= std::tanh(m[static_cast<std::size_t>(j)][static_cast<std::size_t>(ii - 1)] / 2.0);
                }
                // Guard against the (rare) exact a==1.0 case, matching
                // the reference implementation's own clamp - avoids a
                // log(0) division-by-zero singularity.
                if (a > 0.99999) {
                    a = 0.99;
                }
                if (a < -0.99999) {
                    a = -0.99;
                }
                e[static_cast<std::size_t>(j)][static_cast<std::size_t>(i - 1)] = std::log((1.0 + a) / (1.0 - a));
            }
        }

        // Combine each bit's 3 check-messages with its original
        // channel value to get the current best-guess log-likelihood,
        // and check whether that hard-decision codeword already
        // satisfies every parity check.
        std::vector<int> candidate(174);
        for (int i = 0; i < 174; ++i) {
            const auto &checks = kMn[static_cast<std::size_t>(i)];
            double ll = softBits[static_cast<std::size_t>(i)];
            for (int c : checks) {
                ll += e[static_cast<std::size_t>(c - 1)][static_cast<std::size_t>(i)];
            }
            candidate[static_cast<std::size_t>(i)] = (ll < 0.0) ? 1 : 0;
        }
        if (checkParity(candidate)) {
            return std::vector<int>(candidate.begin(), candidate.begin() + 91);
        }

        // Messages from bits to checks for the next iteration: for each
        // of a bit's 3 checks, the outgoing message excludes that
        // check's own contribution (standard belief-propagation
        // extrinsic-information rule - a check must not receive back
        // information that originated from itself).
        for (int i = 0; i < 174; ++i) {
            const auto &checks = kMn[static_cast<std::size_t>(i)];
            for (int excludeSlot = 0; excludeSlot < 3; ++excludeSlot) {
                double ll = softBits[static_cast<std::size_t>(i)];
                for (int otherSlot = 0; otherSlot < 3; ++otherSlot) {
                    if (otherSlot == excludeSlot) {
                        continue;
                    }
                    const int c = checks[static_cast<std::size_t>(otherSlot)];
                    ll += e[static_cast<std::size_t>(c - 1)][static_cast<std::size_t>(i)];
                }
                const int targetCheck = checks[static_cast<std::size_t>(excludeSlot)];
                m[static_cast<std::size_t>(targetCheck - 1)][static_cast<std::size_t>(i)] = ll;
            }
        }
    }

    return std::nullopt;
}

} // namespace psk::dsp::ft8
