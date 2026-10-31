// Einfache DSP-Tests ohne Framework: läuft in CI über ctest.
#include <cmath>
#include <cstdio>
#include <vector>

#include "Presets.h"
#include "dsp/LeimDSP.h"

namespace
{
    int failures = 0;

    void check (bool ok, const char* what)
    {
        std::printf ("%s  %s\n", ok ? "OK  " : "FAIL", what);
        if (! ok) ++failures;
    }

    // Kick + Bass grob nachgebaut: 2 Takte bei 130 BPM
    std::vector<float> testLoop (double sr)
    {
        const int n = (int) (sr * 3.7);
        std::vector<float> x ((size_t) n, 0.0f);
        const double beat = 60.0 / 130.0;
        for (int b = 0; b < 8; ++b)
        {
            const int start = (int) (b * beat * sr);
            double ph = 0;
            for (int i = 0; i < (int) (0.4 * sr) && start + i < n; ++i)
            {
                const double t = i / sr;
                ph += 2 * leim::kPi * (47 + 120 * std::exp (-t / 0.03)) / sr;
                x[(size_t) (start + i)] += (float) (0.6 * std::sin (ph) * std::exp (-t / 0.2));
            }
        }
        for (int i = 0; i < n; ++i)
            x[(size_t) i] += (float) (0.25 * std::sin (2 * leim::kPi * 43.65 * i / sr));
        return x;
    }

    struct Result { double peak = 0, rms = 0; bool finite = true; };

    Result run (const leim::Params& p, const std::vector<float>& in, double sr)
    {
        leim::LeimDSP dsp;
        dsp.setParams (p);
        dsp.prepare (sr);
        std::vector<float> l (in), r (in);
        for (size_t i = 0; i < l.size(); i += 256)
        {
            const int n = (int) std::min<size_t> (256, l.size() - i);
            dsp.process (l.data() + i, r.data() + i, n);
        }
        Result res;
        double sq = 0;
        for (auto v : l)
        {
            if (! std::isfinite (v)) res.finite = false;
            res.peak = std::max (res.peak, (double) std::abs (v));
            sq += (double) v * v;
        }
        res.rms = std::sqrt (sq / (double) l.size());
        return res;
    }

    double rmsOf (const std::vector<float>& x)
    {
        double sq = 0;
        for (auto v : x) sq += (double) v * v;
        return std::sqrt (sq / (double) x.size());
    }
}

int main()
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const auto loop = testLoop (sr);
        std::printf ("\n--- %.0f Hz ---\n", sr);

        // Neutral: Pegel bleibt erhalten
        leim::Params neutral;
        neutral.threshold = 0; neutral.makeup = 0; neutral.satMix = 0; neutral.loDrive = 0; neutral.hiDrive = 0;
        neutral.softClip = false; neutral.scHpf = 0;
        const auto rn = run (neutral, loop, sr);
        check (rn.finite, "Neutral: keine NaN/Inf");
        check (std::abs (20 * std::log10 (rn.rms / rmsOf (loop))) < 0.5, "Neutral: Pegel innerhalb ±0,5 dB");

        // Alle Presets laufen stabil
        bool allFinite = true, allSane = true;
        for (auto& g : leim::presetGenres())
            for (auto& pr : g.presets)
            {
                leim::Params p;
                p.threshold = pr.threshold; p.ratio = pr.ratio; p.attackMs = pr.attackMs; p.releaseS = pr.releaseS;
                p.makeup = pr.makeup; p.range = pr.range; p.scHpf = pr.scHpf; p.compMix = pr.compMix;
                p.xover = pr.xover; p.loStyle = pr.loStyle; p.loDrive = pr.loDrive; p.loLevel = pr.loLevel;
                p.hiStyle = pr.hiStyle; p.hiDrive = pr.hiDrive; p.hiLevel = pr.hiLevel; p.satMix = pr.satMix;
                p.order = pr.order; p.outGain = pr.outGain; p.softClip = pr.softClip;
                const auto r = run (p, loop, sr);
                if (! r.finite) { allFinite = false; std::printf ("  NaN in %s / %s\n", g.name, pr.name); }
                if (r.peak > 4.0 || r.rms < 1e-3) { allSane = false; std::printf ("  Pegel auffällig in %s / %s (peak %.2f)\n", g.name, pr.name, r.peak); }
            }
        check (allFinite, "Alle Presets: keine NaN/Inf");
        check (allSane, "Alle Presets: Pegel plausibel");
    }

    // Statische Kennlinie: Sinus −6 dBFS, Threshold −20, Ratio 4 → etwa 10 dB GR
    {
        const double sr = 48000;
        leim::LeimDSP dsp;
        leim::Params p;
        p.threshold = -20; p.ratio = 4; p.attackMs = 10; p.releaseS = 0.2; p.makeup = 0; p.range = 24;
        p.scHpf = 0; p.satMix = 0; p.loDrive = 0; p.hiDrive = 0; p.softClip = false; p.oversampling = 1;
        dsp.setParams (p);
        dsp.prepare (sr);
        std::vector<float> l ((size_t) sr), r;
        for (size_t i = 0; i < l.size(); ++i) l[i] = (float) (0.5 * std::sin (2 * leim::kPi * 100.0 * (double) i / sr));
        r = l;
        dsp.process (l.data(), r.data(), (int) l.size());
        const double gr = dsp.currentGainReduction();
        std::printf ("\nGR bei Sinus −6 dBFS: %.2f dB\n", gr);
        check (gr > 8.5 && gr < 11.0, "Kompressor: Gain Reduction im erwarteten Bereich");
    }

    std::printf ("\n%s (%d Fehler)\n", failures == 0 ? "Alle Tests bestanden" : "Tests fehlgeschlagen", failures);
    return failures == 0 ? 0 : 1;
}
