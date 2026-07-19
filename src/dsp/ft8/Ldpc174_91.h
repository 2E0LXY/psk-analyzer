#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace psk::dsp::ft8 {

// LDPC(174,91) encoder/decoder for FT8/FT4 - see Ldpc174_91_data.h for the
// real generator/parity matrices this uses (transcribed from an
// established open-source implementation that itself sources them from
// WSJT-X, not independently derived - see that file's comment for why
// that matters for real interoperability).
class Ldpc174_91 {
public:
    // messageBits: 91 bits (77-bit message + 14-bit CRC), each 0 or 1.
    // Returns 174 bits: the 91 message bits unchanged (systematic code),
    // followed by 83 parity bits.
    static std::vector<int> encode(const std::vector<int> &messageBits);

    // softBits: 174 log-likelihood-ratio values, one per codeword bit,
    // using WSJT-X's own sign convention: positive means "more likely a
    // 0", negative means "more likely a 1" (matches the reference
    // implementation's ldpc_decode_python, not an arbitrary choice).
    // Belief-propagation (sum-product) decoding, up to maxIterations
    // rounds. Returns the 91 decoded message bits on success (all 83
    // parity checks satisfied), or std::nullopt if it didn't converge.
    static std::optional<std::vector<int>> decode(const std::vector<double> &softBits,
                                                    int maxIterations = 30);

    // Returns true if all 83 parity checks pass for this 174-bit hard-
    // decision codeword.
    static bool checkParity(const std::vector<int> &codeword);
};

} // namespace psk::dsp::ft8
