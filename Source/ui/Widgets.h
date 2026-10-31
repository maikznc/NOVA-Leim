#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "LeimLookAndFeel.h"

class LeimAudioProcessor;
class SpectrumFifo;

//==============================================================================
// Drehregler: Kappe, Bezeichnung, LCD-Wert
class Knob : public juce::Component
{
public:
    enum Size { Small, Medium, Large };

    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label,
          Size, juce::Colour tone, bool bipolar = false);

    static int dialSize (Size s) { return s == Large ? 74 : s == Medium ? 54 : 42; }
    int preferredWidth() const   { return size == Large ? 96 : size == Medium ? 66 : 60; }
    int preferredHeight() const  { return dialSize (size) + 44; }

    void resized() override;

private:
    juce::Slider slider;
    juce::Label name, value;
    Size size;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Tastenreihe für Auswahl-Parameter (Ratio, Kennlinie, Oversampling)
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (juce::RangedAudioParameter&, const juce::StringArray& labels, int columns,
                   juce::Colour tone = juce::Colours::transparentBlack, bool inWell = false, bool mono = false);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void select (int index);

    juce::OwnedArray<juce::TextButton> buttons;
    int columns;
    bool inWell;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// Leuchttaste für Schalter (Pegelausgleich, Soft Clip, Bypass)
class LampButton : public juce::ToggleButton
{
public:
    LampButton (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text,
                bool redLamp = false, bool dark = false);

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// Umschalter für einen Auswahl-Parameter mit zwei Zuständen (Reihenfolge)
class ChoiceToggle : public juce::ToggleButton
{
public:
    ChoiceToggle (juce::RangedAudioParameter&, juce::StringArray labels, bool dark = false);

private:
    juce::StringArray labels;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// Gain-Reduction-Nadel
class GainReductionMeter : public juce::Component
{
public:
    void update (float targetGr, double dt);
    void paint (juce::Graphics&) override;

private:
    static float grToPos (float gr);
    float pos = 0.0f, vel = 0.0f, shownGr = 0.0f, drawnPos = -1.0f;
};

//==============================================================================
// Ein-/Ausgangspegel mit Peak-Hold
class LevelMeters : public juce::Component
{
public:
    void update (float inPeak, float outPeak, double dt);
    void paint (juce::Graphics&) override;

private:
    struct Bar { float db = -120.0f, hold = -120.0f; double holdTime = 0.0; };
    void step (Bar&, float peak, double dt);
    void paintBar (juce::Graphics&, juce::Rectangle<float>, const Bar&, const juce::String& label);
    Bar in, out;
    double now = 0.0;
};

//==============================================================================
// Spektrum mit ziehbarer Crossover-Linie
class SpectrumView : public juce::Component
{
public:
    explicit SpectrumView (LeimAudioProcessor&);

    void update();   // ~30 Hz aus dem Editor-Timer
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    static constexpr int fftOrder = 13, fftSize = 1 << fftOrder;
    static constexpr float dbTop = 0.0f, dbBottom = -96.0f;

    struct Channel
    {
        std::vector<float> history = std::vector<float> ((size_t) fftSize, 0.0f);
        int writePos = 0;
        std::vector<float> smoothedDb = std::vector<float> ((size_t) fftSize / 2, -120.0f);
    };

    void pull (Channel&, SpectrumFifo&);
    void analyse (Channel&);
    juce::Path curve (const Channel&, juce::Rectangle<float>, bool closed) const;
    float xForFreq (float f, float w) const;
    float freqForX (float x, float w) const;
    void setXoverFromMouse (const juce::MouseEvent&);

    LeimAudioProcessor& processor;
    juce::RangedAudioParameter& xoverParam;
    juce::ParameterAttachment xoverAttachment;
    float xover = 120.0f;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::blackman, true };
    std::vector<float> fftData = std::vector<float> ((size_t) fftSize * 2, 0.0f);
    std::vector<float> scratch = std::vector<float> (32768, 0.0f);
    Channel chIn, chOut;
    bool hasSignal = false;
};

//==============================================================================
// Kennlinie eines Bandes
class CurveView : public juce::Component
{
public:
    explicit CurveView (juce::Colour toneColour) : tone (toneColour) {}
    void setShape (int style, float driveDb, float levelDb, bool autoGain);
    void paint (juce::Graphics&) override;

private:
    juce::Colour tone;
    int style = 0; float drive = 0, level = 0; bool autoGain = true, valid = false;
};
