#pragma once

#include <cstdint>
#include <vector>

namespace psk::dsp {

// Rate-1/2, constraint-length-7 convolutional code, generator polynomials
// G1=0o171, G2=0o133 - the classic NASA/Voyager-heritage code, also used
// (with much larger constraint length) as the basis for WSPR's FEC. K=7
// keeps the trellis to 64 states, small enough for a real, correct,
// soft-decision Viterbi decoder to be implemented and verified in full
// here, unlike WSPR's own K=32 code which requires a Fano sequential
// decoder rather than full Viterbi (2^31 states is not viable to decode
// exactly). Not a novel design - deliberately reusing a well-characterised,
// well-documented code rather than inventing one.
class ConvCode {
public:
    static constexpr int kConstraintLength = 7;
    static constexpr int kNumStates = 1 << (kConstraintLength - 1);
    static constexpr std::uint8_t kGenerator1 = 0b1111001; // 0o171
    static constexpr std::uint8_t kGenerator2 = 0b1011011; // 0o133

    // Encodes bits (0/1 per element), returns 2x as many output bits
    // (rate 1/2), plus (kConstraintLength - 1) trailing zero flush bits'
    // worth of extra output appended, matching standard convolutional
    // code framing (encoder is reset to the all-zero state at both ends).
    static std::vector<int> encode(const std::vector<int> &bits);

    // Soft-decision Viterbi decoding. `softBits` are one value per encoded
    // output bit, positive meaning "more likely a 1", negative meaning
    // "more likely a 0" (e.g. the correlator's real-valued output before
    // hard decision) - NOT hard 0/1 values. Returns the decoded message
    // bits (with the trailing flush bits already stripped).
    static std::vector<int> decodeSoft(const std::vector<double> &softBits, int messageBitCount);
};

} // namespace psk::dsp
