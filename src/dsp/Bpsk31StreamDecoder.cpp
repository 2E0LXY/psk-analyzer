#include "Bpsk31StreamDecoder.h"
#include "PskVaricode.h"

namespace psk::dsp {

Bpsk31StreamDecoder::Bpsk31StreamDecoder(Bpsk31Config config)
    : m_codec(config)
{
}

std::string Bpsk31StreamDecoder::pushSamples(const std::vector<double> &newSamples)
{
    m_buffer.insert(m_buffer.end(), newSamples.begin(), newSamples.end());

    if (!m_acquired) {
        tryAcquire();
        return m_decodedText;
    }

    // Already locked: continue tracking from the stored state using ONLY
    // the newly-arrived samples appended to the buffer - trackStreaming()
    // picks up exactly where it left off (state->symbolStart already
    // points past everything already decoded), so this does not
    // re-process old audio.
    const std::vector<int> newBits = m_codec.trackStreaming(m_buffer, m_lockedOffsetHz, m_state);
    m_allBits.insert(m_allBits.end(), newBits.begin(), newBits.end());
    m_decodedText = PskVaricode::decodeTextBits(m_allBits);
    return m_decodedText;
}

void Bpsk31StreamDecoder::tryAcquire()
{
    const double bufferSeconds = static_cast<double>(m_buffer.size()) / m_codec.config().sampleRate;
    const Bpsk31DemodResult result = m_codec.demodulateTextWithLock(m_buffer);
    if (result.hasLock) {
        m_acquired = true;
        m_lockedOffsetHz = result.lockedOffsetHz;
        // One-time re-run of the (now-known-good) accumulated buffer
        // through the streaming tracker, seeded at the winning offset,
        // starting from a fresh Bpsk31TrackState. This is deliberately
        // not an attempt to transplant demodulateTextWithLock()'s
        // internal loop state directly (it doesn't expose one - it's a
        // batch function) - re-tracking the same, already-accumulated
        // audio once at the moment of acquisition is a small, one-off
        // cost, and it leaves m_state positioned correctly at the end of
        // the current buffer, ready to continue with whatever new
        // samples arrive next.
        m_allBits = m_codec.trackStreaming(m_buffer, m_lockedOffsetHz, m_state);
        m_decodedText = PskVaricode::decodeTextBits(m_allBits);
        return;
    }

    if (bufferSeconds > kMaxUnacquiredBufferSeconds) {
        // Nothing has locked in a generous window - keep only the most
        // recent portion rather than growing the buffer (and therefore
        // the cost of every future failed acquisition attempt, which
        // re-scans the whole thing) without bound while nothing is being
        // received.
        const auto sampleRate = m_codec.config().sampleRate;
        const auto keepSamples = static_cast<std::size_t>(kMaxUnacquiredBufferSeconds * sampleRate * 0.5);
        if (m_buffer.size() > keepSamples) {
            m_buffer.erase(m_buffer.begin(), m_buffer.end() - static_cast<std::ptrdiff_t>(keepSamples));
        }
    }
}

bool Bpsk31StreamDecoder::isAcquired() const
{
    return m_acquired;
}

double Bpsk31StreamDecoder::lockedCarrierHz() const
{
    return m_codec.config().carrierHz + m_lockedOffsetHz;
}

void Bpsk31StreamDecoder::reset()
{
    m_buffer.clear();
    m_state = Bpsk31TrackState{};
    m_acquired = false;
    m_lockedOffsetHz = 0.0;
    m_allBits.clear();
    m_decodedText.clear();
}

} // namespace psk::dsp
