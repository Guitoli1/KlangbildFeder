#pragma once

// DSP für Klangbild Feder: Federhall mit drei Tank-Typen.
// Jeder Tank ist eine Kaskade gestreckter Allpässe (Dispersion -> typisches Zwitschern)
// in einer Rückkopplungsschleife (Laufzeit der Feder), nach Välimäki/Parker/Abel.

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "EQDsp.h"

namespace feder
{
constexpr double twoPi = 6.283185307179586;

//==============================================================================
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int n = 1;
        while (n < maxDelaySamples + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
        maxDelay = (float) (n - 8);
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // Liest das Signal von vor d Samples (d >= 3), Hermite-interpoliert
    float read (float d) const noexcept
    {
        d = std::clamp (d, 3.0f, maxDelay);
        const float rp = (float) w - d;
        const float fl = std::floor (rp);
        const int i = (int) fl;
        const float f = rp - fl;
        const float xm1 = buf[(size_t) ((i - 1) & mask)], x0 = buf[(size_t) (i & mask)];
        const float x1 = buf[(size_t) ((i + 1) & mask)], x2 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

private:
    std::vector<float> buf;
    int mask = 0, w = 0;
    float maxDelay = 0;
};

struct OnePoleLP
{
    float a = 0, z = 0;
    void setCutoff (double fc, double sr) { a = (float) std::exp (-twoPi * fc / sr); }
    float process (float x) noexcept { z = x + a * (z - x); return z; }
    void reset() { z = 0; }
};

struct OnePoleHP
{
    OnePoleLP lp;
    void setCutoff (double fc, double sr) { lp.setCutoff (fc, sr); }
    float process (float x) noexcept { return x - lp.process (x); }
    void reset() { lp.reset(); }
};

struct Lfo
{
    double phase = 0, inc = 0;
    void set (double hz, double sr, double startPhase = 0) { inc = hz / sr; phase = startPhase; }
    float next() noexcept
    {
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        return (float) std::sin (twoPi * phase);
    }
};

//==============================================================================
// Ein "Federtank": Kaskade gestreckter Allpässe (Dispersion -> typisches Zwitschern)
// in einer Rückkopplungsschleife (Laufzeit der Feder), nach Välimäki/Parker/Abel.
class SpringTank
{
public:
    static constexpr int stages = 50;

    void prepare (double sr, double loopSeconds, float allpassCoeff, double modHz)
    {
        fs = sr;
        K = std::max (1, (int) std::lround (sr / (2.0 * 4300.0)));
        a = allpassCoeff;
        apX.assign ((size_t) (stages * K), 0.0f);
        apY.assign ((size_t) (stages * K), 0.0f);
        loopSamples = (float) (loopSeconds * sr);
        loop.prepare ((int) (loopSamples + 0.002 * sr) + 16);
        mod.set (modHz, sr);
        dc.setCutoff (30.0, sr);
        reset();
    }

    void reset()
    {
        std::fill (apX.begin(), apX.end(), 0.0f);
        std::fill (apY.begin(), apY.end(), 0.0f);
        loop.reset(); lp1.reset(); lp2.reset(); dc.reset();
        fb = 0; p = 0;
    }

    double transitionFreq() const { return fs / (2.0 * K); }
    double loopDelaySeconds() const
    {
        // Schleifenverzögerung + mittlere Gruppenlaufzeit der Allpässe
        const double gd = stages * K * 0.5 * ((1 - a) / (1 + a) + (1 + a) / (1 - a));
        return (loopSamples + gd) / fs;
    }

    void setChirp (float coeff) { a = coeff; }

    void setTone (double fc)
    {
        fc = std::min (fc, transitionFreq() * 0.9);
        lp1.setCutoff (fc, fs);
        lp2.setCutoff (fc, fs);
    }

    float process (float x, float g) noexcept
    {
        float v = x + g * fb;
        const size_t base = (size_t) p;
        for (int m = 0; m < stages; ++m)
        {
            const size_t idx = (size_t) (m * K) + base;
            const float y = a * v + apX[idx] - a * apY[idx];
            apX[idx] = v;
            apY[idx] = y;
            v = y;
        }
        if (++p >= K) p = 0;

        v = dc.process (lp2.process (lp1.process (v)));
        loop.push (v);
        fb = loop.read (loopSamples + (float) fs * 0.00025f * mod.next());
        return v;
    }

private:
    double fs = 48000;
    int K = 6, p = 0;
    float a = 0.6f;
    std::vector<float> apX, apY;
    DelayLine loop;
    float loopSamples = 1000, fb = 0;
    OnePoleLP lp1, lp2;
    OnePoleHP dc;
    Lfo mod;
};


// Ein vollständiger Hallkörper mit 2 oder 3 Federn
class SpringUnit
{
public:
    struct Spring { double loop; float coeff; double modHz; };

    void prepare (double sr, std::vector<Spring> springs, float outScale, double decayCalibration)
    {
        fs = sr;
        calib = decayCalibration;
        defs = std::move (springs);
        scale = outScale;
        for (size_t i = 0; i < defs.size(); ++i)
            tanks[i].prepare (sr, defs[i].loop, defs[i].coeff, defs[i].modHz);
        count = (int) defs.size();
        reset();
    }
    void reset() { for (auto& t : tanks) t.reset(); }

    void setParams (float decaySeconds, float tone, float chirp)
    {
        const double t = decaySeconds * 2.2 * calib;  // gleicht Verluste der Tiefpässe aus (je Tank gemessen)
        const double fc = 1500.0 * std::pow (4.0, (double) tone);      // 1,5 – 6 kHz
        for (int i = 0; i < count; ++i)
        {
            tanks[(size_t) i].setChirp (std::clamp (defs[(size_t) i].coeff + (chirp - 0.5f) * 0.18f, 0.40f, 0.78f));
            g[(size_t) i] = (float) std::pow (10.0, -3.0 * tanks[(size_t) i].loopDelaySeconds() / t);
            tanks[(size_t) i].setTone (fc);
        }
    }

    inline void process (float x, float& l, float& r) noexcept
    {
        const float a = tanks[0].process (x, g[0]), b = tanks[1].process (x, g[1]);
        if (count == 3)
        {
            const float c = tanks[2].process (x, g[2]);
            l = scale * (a + 0.6f * c);
            r = scale * (b - 0.6f * c);
        }
        else
        {
            l = scale * (a + 0.25f * b);
            r = scale * (b - 0.25f * a);
        }
    }

private:
    double fs = 48000;
    std::vector<Spring> defs;
    std::array<SpringTank, 3> tanks;
    std::array<float, 3> g { 0.9f, 0.9f, 0.9f };
    int count = 3;
    float scale = 1.0f;
    double calib = 1.0;
};
} // namespace feder
