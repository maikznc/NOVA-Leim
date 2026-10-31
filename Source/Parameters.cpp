#include "Parameters.h"

namespace
{
    const juce::String minus = U ("\xe2\x88\x92"); // U+2212

    juce::String num (float v, int decimals)
    {
        auto s = juce::String (std::abs (v), decimals).replaceCharacter ('.', ',');
        return (v < -0.0001f ? minus : juce::String()) + s;
    }

    juce::String signedNum (float v, int decimals)
    {
        auto s = juce::String (std::abs (v), decimals).replaceCharacter ('.', ',');
        return (v > 0.0001f ? "+" : v < -0.0001f ? minus : juce::String()) + s;
    }

    float parse (const juce::String& t)
    {
        return t.replace (minus, "-").replaceCharacter (',', '.').retainCharacters ("-+0123456789.").getFloatValue();
    }

    using FloatRange = juce::NormalisableRange<float>;

    std::unique_ptr<juce::AudioParameterFloat> dbParam (const char* id, const char* name, float lo, float hi, float def,
                                                        float step, bool showPlus)
    {
        auto attrs = juce::AudioParameterFloatAttributes()
                         .withLabel ("dB")
                         .withStringFromValueFunction ([showPlus] (float v, int) { return (showPlus ? signedNum (v, 1) : num (v, 1)) + " dB"; })
                         .withValueFromStringFunction ([] (const juce::String& t) { return parse (t); });
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, U (name), FloatRange (lo, hi, step), def, attrs);
    }

    std::unique_ptr<juce::AudioParameterFloat> pctParam (const char* id, const char* name, float def)
    {
        auto attrs = juce::AudioParameterFloatAttributes()
                         .withLabel ("%")
                         .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; })
                         .withValueFromStringFunction ([] (const juce::String& t) { return parse (t); });
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, U (name), FloatRange (0.0f, 100.0f, 1.0f), def, attrs);
    }

    template <size_t N>
    juce::StringArray labels (const std::array<double, N>& values, std::function<juce::String (double)> f)
    {
        juce::StringArray s;
        for (auto v : values) s.add (f (v));
        return s;
    }
}

juce::StringArray choices::styleNames() { return { "Tape", "Tube", "Trafo", "Diode", "Clip", "Fold" }; }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto decimal = [] (double v)
    {
        if (std::abs (v - std::round (v)) < 1e-9) return String ((int) std::round (v));
        return String (v, v < 0.1 ? 2 : 1).replaceCharacter ('.', ',');
    };

    // Kompressor
    layout.add (dbParam (ids::threshold, "Threshold", -40.0f, 0.0f, -16.0f, 0.1f, false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::ratio, 1 }, "Ratio", StringArray { "2:1", "4:1", "10:1" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::attack, 1 }, "Attack",
                    labels (choices::attack, [decimal] (double v) { return decimal (v) + " ms"; }), 5));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::release, 1 }, "Release",
                    labels (choices::release, [decimal] (double v) { return v < 0 ? String ("Auto") : decimal (v) + " s"; }), 6));
    layout.add (dbParam (ids::makeup, "Makeup", 0.0f, 20.0f, 2.0f, 0.1f, true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::range, 1 }, "Range", FloatRange (1.0f, 24.0f, 0.5f), 12.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")
                        .withStringFromValueFunction ([] (float v, int) { return num (v, 1) + " dB"; })
                        .withValueFromStringFunction ([] (const String& t) { return parse (t); })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::scHpf, 1 }, "SC-HPF",
                    labels (choices::scHpf, [] (double v) { return v <= 0 ? String ("Aus") : String ((int) v) + " Hz"; }), 3));
    layout.add (pctParam (ids::compMix, "Komp Mix", 100.0f));

    // Sättigung
    FloatRange xoRange (40.0f, 400.0f,
                   [] (float lo, float hi, float n) { return lo * std::pow (hi / lo, n); },
                   [] (float lo, float hi, float v) { return std::log (v / lo) / std::log (hi / lo); },
                   [] (float lo, float hi, float v) { return jlimit (lo, hi, (float) roundToInt (v)); });
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::xover, 1 }, "Crossover", xoRange, 120.0f,
                    AudioParameterFloatAttributes().withLabel ("Hz")
                        .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })
                        .withValueFromStringFunction ([] (const String& t) { return parse (t); })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::loStyle, 1 }, "Low Stil", choices::styleNames(), leim::Tape));
    layout.add (dbParam (ids::loDrive, "Low Drive", 0.0f, 30.0f, 4.0f, 0.1f, true));
    layout.add (dbParam (ids::loLevel, "Low Level", -12.0f, 12.0f, 0.0f, 0.1f, true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::hiStyle, 1 }, "High Stil", choices::styleNames(), leim::Tube));
    layout.add (dbParam (ids::hiDrive, "High Drive", 0.0f, 30.0f, 8.0f, 0.1f, true));
    layout.add (dbParam (ids::hiLevel, "High Level", -12.0f, 12.0f, 0.0f, 0.1f, true));
    layout.add (pctParam (ids::satMix, "\x53\xc3\xa4ttigung Mix", 70.0f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::autoGain, 1 }, "Pegelausgleich", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::oversampling, 1 }, "Oversampling",
                    StringArray { U ("1\xc3\x97"), U ("2\xc3\x97"), U ("4\xc3\x97") }, 2));

    // Ausgang
    layout.add (dbParam (ids::inGain, "Input", -12.0f, 12.0f, 0.0f, 0.1f, true));
    layout.add (dbParam (ids::outGain, "Output", -24.0f, 12.0f, 0.0f, 0.1f, true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::softClip, 1 }, "Soft Clip", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::order, 1 }, "Reihenfolge",
                    StringArray { U ("Komp \xe2\x86\x92 Sat"), U ("Sat \xe2\x86\x92 Komp") }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::bypass, 1 }, "Bypass", false));

    return layout;
}

//==============================================================================
ParamReader::ParamReader (juce::AudioProcessorValueTreeState& s)
    : threshold (get (s, ids::threshold)), ratio (get (s, ids::ratio)), attack (get (s, ids::attack)),
      release (get (s, ids::release)), makeup (get (s, ids::makeup)), range (get (s, ids::range)),
      scHpf (get (s, ids::scHpf)), compMix (get (s, ids::compMix)), xover (get (s, ids::xover)),
      loStyle (get (s, ids::loStyle)), loDrive (get (s, ids::loDrive)), loLevel (get (s, ids::loLevel)),
      hiStyle (get (s, ids::hiStyle)), hiDrive (get (s, ids::hiDrive)), hiLevel (get (s, ids::hiLevel)),
      satMix (get (s, ids::satMix)), autoGain (get (s, ids::autoGain)), oversampling (get (s, ids::oversampling)),
      inGain (get (s, ids::inGain)), outGain (get (s, ids::outGain)), softClip (get (s, ids::softClip)),
      order (get (s, ids::order)), bypass (get (s, ids::bypass))
{
}

std::atomic<float>* ParamReader::get (juce::AudioProcessorValueTreeState& s, const char* id)
{
    auto* p = s.getRawParameterValue (id);
    jassert (p != nullptr);
    return p;
}

leim::Params ParamReader::read() const noexcept
{
    auto idx = [] (const std::atomic<float>* a, int n) { return juce::jlimit (0, n - 1, (int) std::lround (a->load())); };

    leim::Params p;
    p.threshold    = threshold->load();
    p.ratio        = choices::ratio[(size_t) idx (ratio, 3)];
    p.attackMs     = choices::attack[(size_t) idx (attack, 7)];
    p.releaseS     = choices::release[(size_t) idx (release, 7)];
    p.makeup       = makeup->load();
    p.range        = range->load();
    p.scHpf        = choices::scHpf[(size_t) idx (scHpf, 9)];
    p.compMix      = compMix->load();
    p.xover        = xover->load();
    p.loStyle      = idx (loStyle, leim::NumStyles);
    p.loDrive      = loDrive->load();
    p.loLevel      = loLevel->load();
    p.hiStyle      = idx (hiStyle, leim::NumStyles);
    p.hiDrive      = hiDrive->load();
    p.hiLevel      = hiLevel->load();
    p.satMix       = satMix->load();
    p.autoGain     = autoGain->load() > 0.5f;
    p.oversampling = choices::oversampling[(size_t) idx (oversampling, 3)];
    p.inGain       = inGain->load();
    p.outGain      = outGain->load();
    p.softClip     = softClip->load() > 0.5f;
    p.order        = idx (order, 2);
    p.bypass       = bypass->load() > 0.5f;
    return p;
}
