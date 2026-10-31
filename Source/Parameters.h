#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/LeimDSP.h"

// UTF-8-Literal → juce::String (Umlaute, Pfeile, Minuszeichen)
inline juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace ids
{
    inline constexpr auto threshold = "threshold", ratio = "ratio", attack = "attack", release = "release",
                          makeup = "makeup", range = "range", scHpf = "schpf", compMix = "cmix",
                          xover = "xover", loStyle = "loStyle", loDrive = "loDrive", loLevel = "loLevel",
                          hiStyle = "hiStyle", hiDrive = "hiDrive", hiLevel = "hiLevel", satMix = "smix",
                          autoGain = "autogain", oversampling = "os",
                          inGain = "ingain", outGain = "outgain", softClip = "clip", order = "order", bypass = "bypass";
}

namespace choices
{
    inline constexpr std::array<double, 3> ratio   { 2, 4, 10 };
    inline constexpr std::array<double, 7> attack  { 0.01, 0.1, 0.3, 1, 3, 10, 30 };
    inline constexpr std::array<double, 7> release { 0.1, 0.2, 0.4, 0.6, 0.8, 1.2, -1 };
    inline constexpr std::array<double, 9> scHpf   { 0, 30, 45, 60, 90, 120, 150, 200, 300 };
    inline constexpr std::array<int, 3>    oversampling { 1, 2, 4 };

    template <size_t N>
    int nearestIndex (const std::array<double, N>& a, double v)
    {
        size_t best = 0;
        for (size_t i = 1; i < N; ++i)
            if (std::abs (a[i] - v) < std::abs (a[best] - v)) best = i;
        return (int) best;
    }

    juce::StringArray styleNames();
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Schneller Lesezugriff auf alle Parameter im Audio-Thread
class ParamReader
{
public:
    explicit ParamReader (juce::AudioProcessorValueTreeState& s);
    leim::Params read() const noexcept;

private:
    std::atomic<float>* get (juce::AudioProcessorValueTreeState& s, const char* id);
    std::atomic<float> *threshold, *ratio, *attack, *release, *makeup, *range, *scHpf, *compMix,
                       *xover, *loStyle, *loDrive, *loLevel, *hiStyle, *hiDrive, *hiLevel, *satMix,
                       *autoGain, *oversampling, *inGain, *outGain, *softClip, *order, *bypass;
};
