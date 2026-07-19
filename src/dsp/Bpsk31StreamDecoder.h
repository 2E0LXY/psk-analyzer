#pragma once

#include "Bpsk31Codec.h"

#include <vector>

namespace psk::dsp {

// Wraps Bpsk31Codec to actually support continuous reception. The plain
// Bpsk31Codec::demodulateText()/demodulateTextWithLock() are batch
// functions: every call re-runs the full 5-hypothesis acquisition search
// from scratch, scoring each hypothesis against how well its leading bits
// match the PSK31 idle preamble. That works when a real preamble is
// present at (or near) the start of whatever's being processed, but a
// real, continuous transmission only sends its preamble once, at the very
// start - after that scrolls out of a batch decoder's processing window,
// there is nothing preamble-shaped left for the scoring to lock onto,
// even though the actual signal is still there and perfectly decodable.
//
// Confirmed, not theorised: tested against several real PSK31 recordings
// (see git history for the specific files/URLs). A repeating test message
// decoded cleanly and completely whenever a batch window happened to
// start right at one of its preamble repetitions, and produced garbage
// everywhere else in the same file, at the same measured carrier
// frequency, same measured symbol rate, same (already fixed) matched-
// filter pulse-shape tolerance - the only variable that mattered was
// whether a preamble was present at the start of what was being
// processed. That is exactly the failure mode a stateless batch
// re-acquisition design predicts, and exactly what this class exists to
// fix: once acquired, it carries the Costas/Gardner loop state (see
// Bpsk31TrackState) forward across calls via Bpsk31Codec::trackStreaming(),
// so it can keep decoding new audio without ever needing to see another
// preamble.
class Bpsk31StreamDecoder {
public:
    explicit Bpsk31StreamDecoder(Bpsk31Config config = {});

    // Feed newly-captured audio (only the NEW samples since the last
    // call - this class maintains its own internal buffer, unlike the
    // batch functions which expect the whole signal each time). Returns
    // the full accumulated decoded text so far (same convention as
    // AudioEngine's existing "decoded so far" text handling), not just
    // what's new this call.
    std::string pushSamples(const std::vector<double> &newSamples);

    bool isAcquired() const;
    // Only meaningful once isAcquired() is true.
    double lockedCarrierHz() const;

    // Drops all internal state and starts over - e.g. when the operator
    // manually re-tunes, which invalidates whatever this was tracking.
    void reset();

private:
    // Tries to acquire a lock from the accumulated (but not yet locked)
    // buffer, same 5-hypothesis approach as Bpsk31Codec's own batch
    // acquisition. On success, seeds m_state from the winning hypothesis
    // and switches to streaming mode.
    void tryAcquire();

    Bpsk31Codec m_codec;
    std::vector<double> m_buffer;
    Bpsk31TrackState m_state;
    bool m_acquired = false;
    double m_lockedOffsetHz = 0.0;
    std::vector<int> m_allBits;
    std::string m_decodedText;

    // Bound on how much unacquired buffer to accumulate before giving up
    // and resetting - if nothing has locked in this long, continuing to
    // grow the buffer forever on pure noise/no-signal wastes memory and
    // CPU (each failed acquisition attempt re-scans the whole buffer)
    // for no benefit. 10s matches Bpsk31Config::preambleSymbols's actual
    // duration (64 symbols / 31.25 baud ~= 2s) with generous headroom,
    // not an arbitrary guess.
    static constexpr double kMaxUnacquiredBufferSeconds = 10.0;
};

} // namespace psk::dsp
