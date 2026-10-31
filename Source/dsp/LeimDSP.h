// NOVA LEIM – DSP-Kern
// Glue-artiger VCA-Bus-Kompressor + 2-Band-Sättigung mit Oversampling.
// Bewusst ohne JUCE-Abhängigkeit, damit der Kern separat testbar ist.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace leim
{

constexpr double kPi = 3.14159265358979323846;

enum Style : int { Tape = 0, Tube, Trafo, Diode, Clip, Fold, NumStyles };

struct Params
{
    // Kompressor
    double threshold = -16.0;   // dBFS
    double ratio     = 2.0;     // 2, 4, 10
    double attackMs  = 10.0;    // 0.01 … 30
    double releaseS  = -1.0;    // < 0 = Auto
    double makeup    = 2.0;     // dB
    double range     = 12.0;    // max. Gain Reduction in dB
    double scHpf     = 60.0;    // 0 = aus
    double compMix   = 100.0;   // %

    // Sättigung
    double xover   = 120.0;     // Hz
    int    loStyle = Tape;
    double loDrive = 4.0, loLevel = 0.0;
    int    hiStyle = Tube;
    double hiDrive = 8.0, hiLevel = 0.0;
    double satMix  = 70.0;      // %
    bool   autoGain = true;
    int    oversampling = 4;    // 1, 2, 4

    // Ausgang
    double inGain = 0.0, outGain = 0.0;
    bool   softClip = true;
    int    order = 0;           // 0 = Komp → Sat, 1 = Sat → Komp
    bool   bypass = false;

    bool operator== (const Params&) const = default;
};

//==============================================================================
struct Coefs   { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
struct BqState { double z1 = 0, z2 = 0; void reset() noexcept { z1 = z2 = 0; } };

inline double biquad (double x, const Coefs& c, BqState& s) noexcept
{
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

inline Coefs lowpass (double f, double fs, double q) noexcept
{
    const double w = 2.0 * kPi * f / fs, c = std::cos (w), a = std::sin (w) / (2.0 * q), a0 = 1.0 + a;
    return { (1 - c) / 2 / a0, (1 - c) / a0, (1 - c) / 2 / a0, -2 * c / a0, (1 - a) / a0 };
}

inline Coefs highpass (double f, double fs, double q) noexcept
{
    const double w = 2.0 * kPi * f / fs, c = std::cos (w), a = std::sin (w) / (2.0 * q), a0 = 1.0 + a;
    return { (1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0, -2 * c / a0, (1 - a) / a0 };
}

inline double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

//==============================================================================
// Kennlinien
inline double shape (int style, double x) noexcept
{
    constexpr double tanhBias = 0.3363755443363322;   // tanh(0.35)
    constexpr double trafoOff = 0.09985185185185186;  // Kennlinie bei x = 0

    switch (style)
    {
        case Tape:  return std::tanh (x);
        case Tube:  return std::tanh (x + 0.35) - tanhBias;
        case Trafo:
        {
            double v = (x + 0.1) / 1.5;
            v = std::clamp (v, -1.0, 1.0);
            return 1.5 * (v - v * v * v / 3.0) - trafoOff;
        }
        case Diode: return x >= 0.0 ? 1.0 - std::exp (-x) : (std::exp (1.6 * x) - 1.0) / 1.6;
        case Clip:
        {
            const double a = std::abs (x);
            if (a < 0.9) return x;
            const double y = 0.9 + 0.1 * std::tanh ((a - 0.9) / 0.1);
            return x < 0.0 ? -y : y;
        }
        case Fold:  return std::sin (x);
        default:    return x;
    }
}

// RMS-basierter Pegelausgleich, gemessen an einem Sinus mit −12 dBFS
inline double gainCompensation (int style, double drive) noexcept
{
    constexpr int n = 96;
    constexpr double r = 0.25;
    std::array<double, n> out {};
    double sumIn = 0, mean = 0, sumOut = 0;

    for (int i = 0; i < n; ++i)
    {
        const double v = r * std::sin ((i + 0.5) / n * 2.0 * kPi);
        sumIn += v * v;
        out[(size_t) i] = shape (style, drive * v);
        mean += out[(size_t) i];
    }

    mean /= n;
    for (auto o : out) sumOut += (o - mean) * (o - mean);

    return std::clamp (std::sqrt (sumIn / std::max (sumOut, 1e-12)), 0.05, 4.0);
}

inline double softClip (double x) noexcept
{
    const double a = std::abs (x);
    if (a <= 0.7) return x;
    const double y = 0.7 + 0.3 * std::tanh ((a - 0.7) / 0.3);
    return x < 0.0 ? -y : y;
}

//==============================================================================
class LeimDSP
{
public:
    struct Meter
    {
        float inPeak = 0, outPeak = 0, gainReduction = 0;
        double inSq = 0, outSq = 0, drySq = 0;
        int n = 0;
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        smooth = 1.0 - std::exp (-1.0 / (0.015 * sr));
        dcR = 1.0 - 2.0 * kPi * 5.0 / sr;
        os = 0;
        reset();
        setParams (params);
        cur = tgt;
    }

    void reset() noexcept
    {
        yFast = ySlow = grNow = 0;
        dcxL = dcyL = dcxR = dcyR = 0;
        scL.reset(); scR.reset();
        resetFilters();
        meter = {};
    }

    void setParams (const Params& p) noexcept
    {
        params = p;
        const int newOs = (p.oversampling == 1 || p.oversampling == 2) ? p.oversampling : 4;
        if (newOs != os) { os = newOs; resetFilters(); }

        const double fs = sr * os;
        const double fc = std::min (0.375 * sr, 20000.0);
        static constexpr double bw8[4] { 0.50980, 0.60134, 0.89998, 2.56292 };
        for (int j = 0; j < 4; ++j) osC[(size_t) j] = lowpass (fc, fs, bw8[j]);

        const double xo = std::clamp (p.xover, 20.0, 2000.0);
        xoLp = lowpass  (xo, fs, 0.7071067811865476);
        xoHp = highpass (xo, fs, 0.7071067811865476);

        thr   = p.threshold;
        slope = 1.0 - 1.0 / std::max (1.0, p.ratio);
        rangeDb = p.range;
        aAtt  = std::exp (-1.0 / (std::max (p.attackMs, 0.005) * 0.001 * sr));
        autoRelease = p.releaseS < 0.0;
        aRel  = std::exp (-1.0 / ((autoRelease ? 0.1 : p.releaseS) * sr));
        aAttSlow = std::exp (-1.0 / (0.3 * sr));
        aRelSlow = std::exp (-1.0 / (1.5 * sr));
        if (! autoRelease) ySlow = 0;

        const bool sc = p.scHpf > 0.0;
        if (sc) scC = highpass (p.scHpf, sr, 0.7071067811865476);
        if (sc && ! scOn) { scL.reset(); scR.reset(); }
        scOn = sc;

        stLo = std::clamp (p.loStyle, 0, NumStyles - 1);
        stHi = std::clamp (p.hiStyle, 0, NumStyles - 1);
        tgt.gLo = dbToGain (p.loDrive);
        tgt.gHi = dbToGain (p.hiDrive);
        tgt.cLo = (p.autoGain ? gainCompensation (stLo, tgt.gLo) : 1.0) * dbToGain (p.loLevel);
        tgt.cHi = (p.autoGain ? gainCompensation (stHi, tgt.gHi) : 1.0) * dbToGain (p.hiLevel);
        tgt.inG  = dbToGain (p.inGain);
        tgt.outG = dbToGain (p.outGain);
        tgt.mk   = dbToGain (p.makeup);
        tgt.sMix = p.satMix / 100.0;
        tgt.cMix = p.compMix / 100.0;
        tgt.byp  = p.bypass ? 1.0 : 0.0;
        order = p.order;
        clipOn = p.softClip;
    }

    // In-place. Für Mono left == right übergeben.
    void process (float* left, float* right, int numSamples) noexcept
    {
        const double s = smooth;

        for (int i = 0; i < numSamples; ++i)
        {
            cur.inG  += (tgt.inG  - cur.inG)  * s;  cur.outG += (tgt.outG - cur.outG) * s;
            cur.mk   += (tgt.mk   - cur.mk)   * s;  cur.gLo  += (tgt.gLo  - cur.gLo)  * s;
            cur.gHi  += (tgt.gHi  - cur.gHi)  * s;  cur.cLo  += (tgt.cLo  - cur.cLo)  * s;
            cur.cHi  += (tgt.cHi  - cur.cHi)  * s;  cur.sMix += (tgt.sMix - cur.sMix) * s;
            cur.cMix += (tgt.cMix - cur.cMix) * s;  cur.byp  += (tgt.byp  - cur.byp)  * s;

            const double dl = left[i], dr = right[i];
            double l = dl * cur.inG, r = dr * cur.inG;

            const float pin = (float) std::max (std::abs (l), std::abs (r));
            meter.inPeak = std::max (meter.inPeak, pin);
            meter.inSq  += (l * l + r * r) * 0.5;
            meter.drySq += (dl * dl + dr * dr) * 0.5;

            if (order == 0) { compress (l, r); saturate (ol, orr); }
            else            { saturate (l, r); compress (ol, orr); }

            meter.gainReduction = std::max (meter.gainReduction, (float) grNow);

            l = ol * cur.outG;
            r = orr * cur.outG;
            if (clipOn) { l = softClip (l); r = softClip (r); }

            const double b = cur.byp;
            if (b > 1e-4) { l += (dl - l) * b; r += (dr - r) * b; }

            meter.outPeak = std::max (meter.outPeak, (float) std::max (std::abs (l), std::abs (r)));
            meter.outSq  += (l * l + r * r) * 0.5;
            ++meter.n;

            left[i] = (float) l;
            if (right != left) right[i] = (float) r;
        }
    }

    Meter takeMeter() noexcept { Meter m = meter; meter = {}; return m; }
    double currentGainReduction() const noexcept { return grNow; }

private:
    struct Smoothed { double inG = 1, outG = 1, mk = 1, gLo = 1, gHi = 1, cLo = 1, cHi = 1, sMix = 0, cMix = 1, byp = 0; };

    void resetFilters() noexcept
    {
        for (auto* a : { &upL, &upR, &dnL, &dnR }) for (auto& st : *a) st.reset();
        for (auto* st : { &lpL1, &lpL2, &hpL1, &hpL2, &lpR1, &lpR2, &hpR1, &hpR2 }) st->reset();
    }

    void compress (double l, double r) noexcept
    {
        double dl = l, dr = r;
        if (scOn) { dl = biquad (dl, scC, scL); dr = biquad (dr, scC, scR); }

        const double lv = std::max (std::abs (dl), std::abs (dr));
        const double db = lv > 1e-7 ? 8.685889638065035 * std::log (lv) : -140.0;
        const double over = db - thr;

        double x = 0.0;
        if (over >= kKnee * 0.5)       x = slope * over;
        else if (over > -kKnee * 0.5) { const double q = over + kKnee * 0.5; x = slope * q * q / (2.0 * kKnee); }
        x = std::min (x, rangeDb);

        yFast = x > yFast ? aAtt * yFast + (1 - aAtt) * x : aRel * yFast + (1 - aRel) * x;
        double g = yFast;

        if (autoRelease)
        {
            ySlow = x > ySlow ? aAttSlow * ySlow + (1 - aAttSlow) * x : aRelSlow * ySlow + (1 - aRelSlow) * x;
            g = std::max (g, ySlow);
        }

        grNow = g;
        const double gain = std::exp (-g * 0.1151292546497023) * cur.mk;
        const double m = cur.cMix;
        ol  = l + (l * gain - l) * m;
        orr = r + (r * gain - r) * m;
    }

    void saturate (double l, double r) noexcept
    {
        const double gLo = cur.gLo, gHi = cur.gHi, cLo = cur.cLo, cHi = cur.cHi;
        const double wet = cur.sMix, dry = 1.0 - wet;
        double yl = 0, yr = 0;

        for (int k = 0; k < os; ++k)
        {
            double ul = k == 0 ? l * os : 0.0, ur = k == 0 ? r * os : 0.0;
            if (os > 1)
                for (size_t j = 0; j < 4; ++j) { ul = biquad (ul, osC[j], upL[j]); ur = biquad (ur, osC[j], upR[j]); }

            const double loL = biquad (biquad (ul, xoLp, lpL1), xoLp, lpL2), hiL = biquad (biquad (ul, xoHp, hpL1), xoHp, hpL2);
            const double loR = biquad (biquad (ur, xoLp, lpR1), xoLp, lpR2), hiR = biquad (biquad (ur, xoHp, hpR1), xoHp, hpR2);

            double vl = (loL + hiL) * dry + (shape (stLo, loL * gLo) * cLo + shape (stHi, hiL * gHi) * cHi) * wet;
            double vr = (loR + hiR) * dry + (shape (stLo, loR * gLo) * cLo + shape (stHi, hiR * gHi) * cHi) * wet;

            if (os > 1)
                for (size_t j = 0; j < 4; ++j) { vl = biquad (vl, osC[j], dnL[j]); vr = biquad (vr, osC[j], dnR[j]); }

            yl = vl; yr = vr;
        }

        // DC-Blocker (Tube/Trafo/Diode erzeugen Gleichanteil)
        const double outL = yl - dcxL + dcR * dcyL; dcxL = yl; dcyL = outL;
        const double outR = yr - dcxR + dcR * dcyR; dcxR = yr; dcyR = outR;
        ol = outL; orr = outR;
    }

    static constexpr double kKnee = 6.0;

    Params params;
    double sr = 48000.0, smooth = 0.01, dcR = 0.999;
    int os = 0, order = 0, stLo = 0, stHi = 1;
    bool clipOn = true, autoRelease = true, scOn = false;

    Smoothed tgt, cur;

    // Kompressor
    double thr = -16, slope = 0.5, rangeDb = 12, aAtt = 0, aRel = 0, aAttSlow = 0, aRelSlow = 0;
    double yFast = 0, ySlow = 0, grNow = 0;
    Coefs scC; BqState scL, scR;

    // Sättigung
    std::array<Coefs, 4> osC {};
    std::array<BqState, 4> upL {}, upR {}, dnL {}, dnR {};
    Coefs xoLp, xoHp;
    BqState lpL1, lpL2, hpL1, hpL2, lpR1, lpR2, hpR1, hpR2;
    double dcxL = 0, dcyL = 0, dcxR = 0, dcyR = 0;

    double ol = 0, orr = 0;
    Meter meter;
};

} // namespace leim
