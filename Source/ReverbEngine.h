// ============================================================================
//  Velvet Verb - moteur de reverb (C++ pur, sans dependance)
//  FDN 8 lignes (matrice de Householder) + diffusion d'entree + pre-delay,
//  amortissement des aigus, filtres coupe-bas / coupe-haut, largeur stereo,
//  ducking (la reverb s'efface pendant que la voix chante) et dry/wet.
// ============================================================================
#pragma once

#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <atomic>

namespace velvet
{

struct ReverbParams
{
    float mix        = 0.25f;   // 0..1
    float predelayMs = 20.0f;   // 0..250
    float size       = 0.55f;   // 0..1
    float decaySec   = 1.8f;    // 0.2..12
    float damping    = 0.35f;   // 0..1
    float lowCutHz   = 200.0f;  // 20..1000
    float highCutHz  = 9000.0f; // 1000..20000
    float width      = 1.0f;    // 0..1.5
    float duck       = 0.0f;    // 0..1
};

// ---------------------------------------------------------------------------
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        int size = 1;
        while (size < maxSamples + 4) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }
    void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }

    inline void push (float x) noexcept
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }
    // lecture "delay" echantillons en arriere depuis le dernier push (>= 1)
    inline float read (float delay) const noexcept
    {
        const float pos = (float) writePos - delay;
        const int   i0  = (int) std::floor (pos);
        const float fr  = pos - (float) i0;
        const float a = buffer[(size_t) ( i0      & mask)];
        const float b = buffer[(size_t) ((i0 + 1) & mask)];
        return a + fr * (b - a);
    }
    inline float readInt (int delay) const noexcept
    {
        return buffer[(size_t) ((writePos - delay) & mask)];
    }
private:
    std::vector<float> buffer;
    int mask = 0, writePos = 0;
};

// ---------------------------------------------------------------------------
class Allpass
{
public:
    void prepare (int lenSamples, float gain) { len = std::max (1, lenSamples); g = gain; line.prepare (len + 2); }
    void clear() { line.clear(); }
    inline float process (float x) noexcept
    {
        const float d = line.readInt (len);
        const float v = x + g * d;
        line.push (v);
        return d - g * v;
    }
private:
    DelayLine line; int len = 1; float g = 0.6f;
};

// ---------------------------------------------------------------------------
// Biquad (RBJ) - utilise pour les coupe-bas / coupe-haut du signal reverbere
class Biquad
{
public:
    void setHighPass (double fs, double f) { setup (fs, f, true); }
    void setLowPass  (double fs, double f) { setup (fs, f, false); }
    void clear() { z1 = z2 = 0.0f; }
    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
private:
    void setup (double fs, double f, bool highpass)
    {
        f = std::clamp (f, 10.0, fs * 0.45);
        const double w0 = 2.0 * 3.14159265358979323846 * f / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double q = 0.70710678, alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha;
        double nb0, nb1, nb2;
        if (highpass) { nb0 = (1.0 + cw) * 0.5; nb1 = -(1.0 + cw); nb2 = nb0; }
        else          { nb0 = (1.0 - cw) * 0.5; nb1 =  (1.0 - cw); nb2 = nb0; }
        b0 = (float) (nb0 / a0); b1 = (float) (nb1 / a0); b2 = (float) (nb2 / a0);
        a1 = (float) (-2.0 * cw / a0); a2 = (float) ((1.0 - alpha) / a0);
    }
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

// ---------------------------------------------------------------------------
class ReverbEngine
{
public:
    static constexpr int N = 8;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        fs = sampleRate;

        preDelayL.prepare ((int) (0.26 * fs) + 8);
        preDelayR.prepare ((int) (0.26 * fs) + 8);

        static const float diffL[4] = { 4.77f, 3.59f, 12.73f, 9.30f };
        static const float diffR[4] = { 4.92f, 3.71f, 12.31f, 9.67f };
        for (int i = 0; i < 4; ++i)
        {
            const float g = i < 2 ? 0.70f : 0.62f;
            apL[i].prepare ((int) (diffL[i] * 0.001 * fs), g);
            apR[i].prepare ((int) (diffR[i] * 0.001 * fs), g);
        }

        modDepth = (float) (0.00025 * fs);
        for (int i = 0; i < N; ++i)
        {
            const double maxDelay = baseMs[i] * 0.001 * fs * maxScale + modDepth + 8;
            lines[i].prepare ((int) maxDelay);
            lfoPhase[i] = (float) i / (float) N;
            lfoInc[i]   = (float) (lfoRates[i] / fs);
        }

        smoothCoef  = (float) (1.0 - std::exp (-1.0 / (0.05 * fs)));   // ~50 ms
        sizeCoef    = (float) (1.0 - std::exp (-1.0 / (0.25 * fs)));   // ~250 ms (glissement doux)
        envAttack   = (float) (1.0 - std::exp (-1.0 / (0.005 * fs)));
        envRelease  = (float) (1.0 - std::exp (-1.0 / (0.30 * fs)));
        hpLoopCoef  = (float) std::exp (-2.0 * 3.14159265358979323846 * 30.0 / fs);

        firstBlock = true;
        reset();
    }

    void reset()
    {
        preDelayL.clear(); preDelayR.clear();
        for (auto& a : apL) a.clear();
        for (auto& a : apR) a.clear();
        for (auto& l : lines) l.clear();
        for (int i = 0; i < N; ++i) { lpState[i] = 0; hpX[i] = 0; hpY[i] = 0; }
        hpL.clear(); hpR.clear(); lpL.clear(); lpR.clear();
        env = 0;
        lastLowCut = lastHighCut = -1.0f;
    }

    void setParams (const ReverbParams& p) { target = p; }

    // traitement stereo sur place
    void process (float* left, float* right, int numSamples)
    {
        if (firstBlock)
        {
            cur = target;
            curPreDelay = target.predelayMs * 0.001f * (float) fs;
            curScale = sizeToScale (target.size);
            curDuckGain = 1.0f;
            firstBlock = false;
        }

        // coefficients mis a jour une fois par bloc
        if (target.lowCutHz != lastLowCut)   { hpL.setHighPass (fs, target.lowCutHz); hpR.setHighPass (fs, target.lowCutHz); lastLowCut = target.lowCutHz; }
        if (target.highCutHz != lastHighCut) { lpL.setLowPass  (fs, target.highCutHz); lpR.setLowPass (fs, target.highCutHz); lastHighCut = target.highCutHz; }

        const float dampFc = 16000.0f * std::pow (0.08f, target.damping);
        const float dampA  = (float) (1.0 - std::exp (-2.0 * 3.14159265358979323846 * dampFc / fs));

        const float targetScale = sizeToScale (target.size);
        const float targetPre   = std::clamp (target.predelayMs, 0.0f, 250.0f) * 0.001f * (float) fs;

        // gains de feedback calcules pour la taille moyenne du bloc
        float gains[N];
        {
            const float scaleMid = curScale + (targetScale - curScale) * 0.5f;
            const double rt = std::max (0.05, (double) target.decaySec) * 1.18; // compense les pertes de la boucle
            for (int i = 0; i < N; ++i)
            {
                const double lenSec = baseMs[i] * 0.001 * scaleMid;
                gains[i] = (float) std::pow (10.0, -3.0 * lenSec / rt);
            }
        }

        const float duckAmt = std::clamp (target.duck, 0.0f, 1.0f);
        float peak = 0.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            // --- lissage des parametres
            cur.mix   += (target.mix   - cur.mix)   * smoothCoef;
            cur.width += (target.width - cur.width) * smoothCoef;
            curScale  += (targetScale - curScale) * sizeCoef;
            curPreDelay += (targetPre - curPreDelay) * smoothCoef;

            const float dryL = left[n], dryR = right[n];

            // --- detection d'enveloppe pour le ducking
            const float lvl = std::max (std::abs (dryL), std::abs (dryR));
            env += (lvl - env) * (lvl > env ? envAttack : envRelease);
            const float duckTarget = 1.0f / (1.0f + duckAmt * 12.0f * env);
            curDuckGain += (duckTarget - curDuckGain) * smoothCoef;

            // --- pre-delay
            preDelayL.push (dryL);
            preDelayR.push (dryR);
            const float pd = std::max (1.0f, curPreDelay);
            float inL = preDelayL.read (pd);
            float inR = preDelayR.read (pd);

            // --- diffusion d'entree
            for (int i = 0; i < 4; ++i) { inL = apL[i].process (inL); inR = apR[i].process (inR); }

            // --- lecture des 8 lignes (modulees tres legerement)
            float o[N];
            for (int i = 0; i < N; ++i)
            {
                lfoPhase[i] += lfoInc[i];
                if (lfoPhase[i] >= 1.0f) lfoPhase[i] -= 1.0f;
                const float mod = modDepth * fastSin (lfoPhase[i]);
                const float d = (float) (baseMs[i] * 0.001 * fs) * curScale + mod + modDepth + 2.0f;
                float x = lines[i].read (d);

                // amortissement des aigus dans la boucle
                lpState[i] += dampA * (x - lpState[i]);
                x = lpState[i];
                // coupe-bas tres doux (30 Hz) dans la boucle pour eviter le grondement
                const float hp = hpLoopCoef * (hpY[i] + x - hpX[i]);
                hpX[i] = x; hpY[i] = hp;
                o[i] = hp;
            }

            // --- sorties stereo (deux lignes de Hadamard decorrelees)
            float wetL = (o[0] - o[1] + o[2] - o[3] + o[4] - o[5] + o[6] - o[7]) * outScale;
            float wetR = (o[0] + o[1] - o[2] - o[3] + o[4] + o[5] - o[6] - o[7]) * outScale;

            // --- feedback : gains de decroissance + Householder
            float s = 0.0f;
            for (int i = 0; i < N; ++i) { o[i] *= gains[i]; s += o[i]; }
            s *= (2.0f / (float) N);
            for (int i = 0; i < N; ++i)
            {
                const float inj = (i & 1) ? inR : inL;
                const float sign = ((i >> 1) & 1) ? -1.0f : 1.0f;
                lines[i].push (o[i] - s + sign * inj * inputScale);
            }

            // --- EQ du signal reverbere
            wetL = lpL.process (hpL.process (wetL));
            wetR = lpR.process (hpR.process (wetR));

            // --- largeur stereo
            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * cur.width;
            wetL = mid + side;
            wetR = mid - side;

            // --- ducking
            wetL *= curDuckGain;
            wetR *= curDuckGain;

            peak = std::max (peak, std::max (std::abs (wetL), std::abs (wetR)));

            // --- dry / wet (le dry reste a 100% jusqu'a 50% de mix)
            const float dryG = std::min (1.0f, 2.0f * (1.0f - cur.mix));
            const float wetG = std::min (1.0f, 2.0f * cur.mix);
            left[n]  = dryL * dryG + wetL * wetG;
            right[n] = dryR * dryG + wetR * wetG;
        }

        // niveau pour l'animation de l'interface
        const float prev = wetLevel.load (std::memory_order_relaxed);
        wetLevel.store (std::max (peak, prev * 0.85f), std::memory_order_relaxed);

        // anti-denormaux
        for (int i = 0; i < N; ++i)
            if (std::abs (lpState[i]) < 1.0e-15f) lpState[i] = 0.0f;
    }

    float getWetLevel() const noexcept { return wetLevel.load (std::memory_order_relaxed); }

private:
    static float sizeToScale (float size)
    {
        size = std::clamp (size, 0.0f, 1.0f);
        return minScale + (maxScale - minScale) * std::pow (size, 1.3f);
    }
    static inline float fastSin (float phase01) noexcept
    {
        // approximation parabolique d'un sinus, phase 0..1
        const float x = phase01 * 2.0f - 1.0f;           // -1..1
        return 4.0f * x * (1.0f - std::abs (x));
    }

    static constexpr float minScale = 0.30f, maxScale = 2.0f;
    static constexpr double baseMs[N]   = { 31.3, 37.9, 41.3, 47.1, 53.9, 61.7, 67.3, 79.1 };
    static constexpr double lfoRates[N] = { 0.31, 0.43, 0.57, 0.67, 0.79, 0.89, 1.03, 1.13 };
    static constexpr float outScale   = 0.42f;
    static constexpr float inputScale = 0.50f;

    double fs = 44100.0;
    ReverbParams target, cur;
    bool firstBlock = true;

    DelayLine preDelayL, preDelayR;
    Allpass apL[4], apR[4];
    DelayLine lines[N];
    float lpState[N] {}, hpX[N] {}, hpY[N] {};
    float lfoPhase[N] {}, lfoInc[N] {};
    float modDepth = 10.0f;

    Biquad hpL, hpR, lpL, lpR;
    float lastLowCut = -1.0f, lastHighCut = -1.0f;

    float smoothCoef = 0.001f, sizeCoef = 0.0001f;
    float envAttack = 0.01f, envRelease = 0.0001f, env = 0.0f;
    float hpLoopCoef = 0.999f;
    float curScale = 1.0f, curPreDelay = 0.0f, curDuckGain = 1.0f;

    std::atomic<float> wetLevel { 0.0f };
};

} // namespace velvet
