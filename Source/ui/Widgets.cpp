#include "Widgets.h"
#include "../PluginProcessor.h"
#include "../dsp/LeimDSP.h"

using namespace juce;

namespace
{
    const String minus = String::fromUTF8 ("\xe2\x88\x92");

    String dbText (float db, int decimals = 1)
    {
        if (db <= -90.0f) return minus + String::fromUTF8 ("\xe2\x88\x9e");
        return (db < -0.05f ? minus : (db > 0.05f ? String ("+") : String())) + String (std::abs (db), decimals).replaceCharacter ('.', ',');
    }

    Colour valueColourFor (Colour tone) { return tone == col::amber ? col::amberHot : tone; }
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& state, const String& paramId, const String& label,
            Size s, Colour tone, bool bipolar)
    : size (s)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setColour (Slider::rotarySliderFillColourId, tone);
    slider.getProperties().set ("bipolar", bipolar);
    slider.setMouseDragSensitivity (220);
    slider.onValueChange = [this] { value.setText (slider.getTextFromValue (slider.getValue()), dontSendNotification); };
    addAndMakeVisible (slider);

    name.setText (label.toUpperCase(), dontSendNotification);
    name.setFont (LeimLookAndFeel::labelFont (11.5f));
    name.setJustificationType (Justification::centred);
    name.setMinimumHorizontalScale (0.72f);
    name.setBorderSize ({});
    name.setColour (Label::textColourId, col::silk);
    name.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (name);

    value.setFont (LeimLookAndFeel::monoFont (12.0f));
    value.setJustificationType (Justification::centred);
    value.setColour (Label::textColourId, valueColourFor (tone));
    value.setColour (Label::backgroundColourId, col::screen);
    value.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (value);

    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    value.setText (slider.getTextFromValue (slider.getValue()), dontSendNotification);
}

void Knob::resized()
{
    const int d = dialSize (size);
    const int w = getWidth();
    slider.setBounds ((w - d) / 2, 0, d, d);
    name.setBounds (0, d + 4, w, 15);
    const int vw = jmin (w - 8, 76);
    value.setBounds ((w - vw) / 2, d + 22, vw, 20);
}

//==============================================================================
ChoiceButtons::ChoiceButtons (RangedAudioParameter& param, const StringArray& labels, int cols,
                              Colour tone, bool well, bool mono)
    : columns (jmax (1, cols)), inWell (well)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->setClickingTogglesState (false);
        b->getProperties().set ("tone", (int64) tone.getARGB());
        b->getProperties().set ("flat", inWell);
        b->getProperties().set ("mono", mono);
        b->onClick = [this, i] { if (attachment != nullptr) attachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    attachment = std::make_unique<ParameterAttachment> (param, [this] (float v) { select (roundToInt (v)); });
    attachment->sendInitialUpdate();
}

void ChoiceButtons::select (int index)
{
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == index, dontSendNotification);
}

void ChoiceButtons::paint (Graphics& g)
{
    if (! inWell) return;
    auto r = getLocalBounds().toFloat();
    g.setColour (col::well);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
}

void ChoiceButtons::resized()
{
    const int n = buttons.size();
    if (n == 0) return;
    const int rows = (n + columns - 1) / columns;
    const int pad = inWell ? 3 : 0, gap = inWell ? 3 : 4;
    auto area = getLocalBounds().reduced (pad);
    const float bw = (float) (area.getWidth()  - gap * (columns - 1)) / (float) columns;
    const float bh = (float) (area.getHeight() - gap * (rows - 1))    / (float) rows;

    for (int i = 0; i < n; ++i)
    {
        const int c = i % columns, r = i / columns;
        buttons[i]->setBounds (Rectangle<float> (area.getX() + c * (bw + gap), area.getY() + r * (bh + gap), bw, bh).toNearestInt());
    }
}

//==============================================================================
LampButton::LampButton (AudioProcessorValueTreeState& state, const String& paramId, const String& buttonText,
                        bool redLamp, bool dark)
    : ToggleButton (buttonText)
{
    getProperties().set ("red", redLamp);
    getProperties().set ("dark", dark);
    attachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (state, paramId, *this);
}

ChoiceToggle::ChoiceToggle (RangedAudioParameter& param, StringArray l, bool dark)
    : labels (std::move (l))
{
    getProperties().set ("dark", dark);
    setClickingTogglesState (false);
    onClick = [this] { if (attachment != nullptr) attachment->setValueAsCompleteGesture (getToggleState() ? 0.0f : 1.0f); };
    attachment = std::make_unique<ParameterAttachment> (param, [this] (float v)
    {
        const bool on = v > 0.5f;
        setToggleState (on, dontSendNotification);
        setButtonText (labels[on ? 1 : 0]);
    });
    attachment->sendInitialUpdate();
}

//==============================================================================
float GainReductionMeter::grToPos (float gr)
{
    static constexpr float marks[][2] { { 0, 0 }, { 1, .11f }, { 2, .21f }, { 3, .3f }, { 4, .385f }, { 6, .53f },
                                        { 8, .645f }, { 10, .74f }, { 14, .875f }, { 20, 1 } };
    if (gr <= 0) return 0;
    for (size_t i = 1; i < std::size (marks); ++i)
        if (gr <= marks[i][0])
            return marks[i - 1][1] + (marks[i][1] - marks[i - 1][1]) * (gr - marks[i - 1][0]) / (marks[i][0] - marks[i - 1][0]);
    return 1;
}

void GainReductionMeter::update (float targetGr, double dt)
{
    const float target = grToPos (targetGr);
    const float w = MathConstants<float>::twoPi * 2.1f, z = 0.8f, t = (float) dt;
    vel += ((target - pos) * w * w - 2.0f * z * w * vel) * t;
    pos += vel * t;
    if (pos < -0.02f) { pos = -0.02f; vel = 0; }
    if (pos > 1.03f)  { pos = 1.03f;  vel = 0; }
    shownGr += (targetGr - shownGr) * (1.0f - std::exp (-t / 0.12f));

    if (std::abs (pos - drawnPos) > 0.0004f)
    {
        drawnPos = pos;
        repaint();
    }
}

void GainReductionMeter::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (col::well);
    g.fillRoundedRectangle (bounds, 7.0f);

    const auto face = bounds.reduced (6.0f);
    const float W = face.getWidth(), H = face.getHeight();

    Graphics::ScopedSaveState save (g);
    Path clip;
    clip.addRoundedRectangle (face, 4.0f);
    g.reduceClipRegion (clip);

    g.setGradientFill (ColourGradient (col::vuHot, face.getCentreX(), face.getBottom() + H * 0.05f,
                                       col::vu, face.getCentreX(), face.getY() - H * 0.35f, true));
    g.fillRect (face);

    const float cx = face.getCentreX(), cy = face.getY() + H * 1.34f, R = H * 1.12f;
    const float span = degreesToRadians (46.0f);
    auto ang = [span] (float q) { return (0.5f - q) * 2.0f * span; };
    auto P = [cx, cy] (float a, float r) { return Point<float> (cx + r * std::sin (a), cy - r * std::cos (a)); };

    Path hot;
    hot.addCentredArc (cx, cy, R * 0.885f, R * 0.885f, 0.0f, ang (1.0f), ang (grToPos (10.0f)), true);
    g.setColour (col::red.withAlpha (0.75f));
    g.strokePath (hot, PathStrokeType (5.0f));

    Path scale;
    scale.addCentredArc (cx, cy, R * 0.86f, R * 0.86f, 0.0f, -span, span, true);
    g.setColour (col::ink);
    g.strokePath (scale, PathStrokeType (1.2f));

    static constexpr float marks[][2] { { 0, 0 }, { 1, .11f }, { 2, .21f }, { 3, .3f }, { 4, .385f }, { 6, .53f },
                                        { 8, .645f }, { 10, .74f }, { 14, .875f }, { 20, 1 } };
    g.setFont (LeimLookAndFeel::monoFont (11.0f));
    for (size_t i = 0; i < std::size (marks); ++i)
    {
        const float a = ang (marks[i][1]);
        g.drawLine ({ P (a, R * 0.86f), P (a, R * 0.935f) }, 1.4f);
        const auto lp = P (a, R * 1.0f);
        g.drawText (String ((int) marks[i][0]), Rectangle<float> (lp.x - 14, lp.y - 7, 28, 14), Justification::centred, false);
        if (i + 1 < std::size (marks))
        {
            const float am = ang ((marks[i][1] + marks[i + 1][1]) * 0.5f);
            g.drawLine ({ P (am, R * 0.86f), P (am, R * 0.9f) }, 1.0f);
        }
    }

    g.setColour (col::ink.withAlpha (0.75f));
    g.setFont (LeimLookAndFeel::labelFont (11.0f));
    g.drawText (String::fromUTF8 ("GAIN REDUCTION  \xc2\xb7  dB"), Rectangle<float> (face.getX(), face.getY() + H * 0.6f - 8, W, 16),
                Justification::centred, false);

    g.setFont (LeimLookAndFeel::monoFont (11.0f));
    g.drawText ((shownGr > 0.05f ? minus : String()) + String (shownGr, 1).replaceCharacter ('.', ','),
                Rectangle<float> (face.getRight() - 70, face.getBottom() - 20, 62, 14), Justification::centredRight, false);
    g.setFont (LeimLookAndFeel::brandFont (9.0f));
    g.drawText ("NOVA", Rectangle<float> (face.getX() + 8, face.getBottom() - 20, 60, 14), Justification::centredLeft, false);

    const float a = ang (pos);
    const auto tip = P (a, R * 0.97f);
    g.setColour (Colours::black.withAlpha (0.18f));
    g.drawLine (cx + 2, cy, tip.x + 2, tip.y + 1, 2.0f);
    g.setColour (col::ink);
    g.drawLine (cx, cy, tip.x, tip.y, 1.6f);

    g.setColour (col::plateLo);
    g.fillEllipse (cx - W * 0.16f, face.getBottom() - H * 0.2f + 2.0f, W * 0.32f, H * 0.4f);

    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.16f), 0, face.getY(),
                                       Colours::transparentBlack, 0, face.getY() + H * 0.35f, false));
    g.fillRect (face);
}

//==============================================================================
void LevelMeters::step (Bar& b, float peak, double dt)
{
    const float db = peak > 1e-6f ? 20.0f * std::log10 (peak) : -120.0f;
    b.db = jmax (db, b.db - 26.0f * (float) dt);
    if (b.db >= b.hold) { b.hold = b.db; b.holdTime = now; }
    else if (now - b.holdTime > 1.2) b.hold = jmax (b.db, b.hold - 20.0f * (float) dt);
}

void LevelMeters::update (float inPeak, float outPeak, double dt)
{
    now += dt;
    step (in, inPeak, dt);
    step (out, outPeak, dt);
    repaint();
}

void LevelMeters::paintBar (Graphics& g, Rectangle<float> r, const Bar& b, const String& label)
{
    auto frac = [] (float db) { return jlimit (0.0f, 1.0f, (db + 48.0f) / 51.0f); };

    g.setColour (col::silkDim);
    g.setFont (LeimLookAndFeel::labelFont (10.5f));
    g.drawText (label, Rectangle<float> (r.getCentreX() - 20, r.getY() - 18, 40, 14), Justification::centred, false);

    g.setColour (col::screen);
    g.fillRoundedRectangle (r, 3.0f);

    ColourGradient grad (col::silkDim, 0, r.getBottom(), col::red, 0, r.getY(), false);
    grad.addColour (0.62, col::silk);
    grad.addColour (0.84, col::amber);
    {
        Graphics::ScopedSaveState s (g);
        const float top = r.getY() + r.getHeight() * (1.0f - frac (b.db));
        g.reduceClipRegion (Rectangle<float> (r.getX(), top, r.getWidth(), r.getBottom() - top).toNearestInt());
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 3.0f);
    }

    if (b.hold > -47.0f)
    {
        const float y = r.getBottom() - r.getHeight() * frac (b.hold);
        g.setColour (b.hold > -0.1f ? col::red : col::amberHot);
        g.fillRect (r.getX(), y - 1.0f, r.getWidth(), 2.0f);
    }

    g.setColour (col::silk);
    g.setFont (LeimLookAndFeel::monoFont (10.5f));
    g.drawText (dbText (b.db), Rectangle<float> (r.getCentreX() - 26, r.getBottom() + 5, 52, 14), Justification::centred, false);
}

void LevelMeters::paint (Graphics& g)
{
    auto area = getLocalBounds().toFloat().withTrimmedTop (20).withTrimmedBottom (22);
    const float barW = 16, scaleW = 28, gap = 6;
    const float x0 = area.getCentreX() - (barW * 2 + scaleW + gap * 2) * 0.5f;
    const auto inR    = Rectangle<float> (x0, area.getY(), barW, area.getHeight());
    const auto scaleR = Rectangle<float> (x0 + barW + gap, area.getY(), scaleW, area.getHeight());
    const auto outR   = Rectangle<float> (scaleR.getRight() + gap, area.getY(), barW, area.getHeight());

    paintBar (g, inR, in, "IN");
    paintBar (g, outR, out, "OUT");

    g.setColour (col::silkDim);
    g.setFont (LeimLookAndFeel::monoFont (9.5f));
    for (int db : { 0, -6, -12, -18, -24, -36, -48 })
    {
        const float y = scaleR.getBottom() - scaleR.getHeight() * ((float) db + 48.0f) / 51.0f;
        g.drawText (db == 0 ? String ("0") : minus + String (-db), Rectangle<float> (scaleR.getX(), y - 6, scaleW, 12), Justification::centred, false);
    }
}

//==============================================================================
SpectrumView::SpectrumView (LeimAudioProcessor& p)
    : processor (p),
      xoverParam (*p.apvts.getParameter (ids::xover)),
      xoverAttachment (xoverParam, [this] (float v) { xover = v; repaint(); })
{
    xoverAttachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::LeftRightResizeCursor);
}

void SpectrumView::pull (Channel& ch, SpectrumFifo& fifo)
{
    const int n = fifo.pull (scratch.data(), (int) scratch.size());
    for (int i = 0; i < n; ++i)
    {
        ch.history[(size_t) ch.writePos] = scratch[(size_t) i];
        ch.writePos = (ch.writePos + 1) % fftSize;
    }
    if (n > 0) hasSignal = true;
}

void SpectrumView::analyse (Channel& ch)
{
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = ch.history[(size_t) ((ch.writePos + i) % fftSize)];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (int k = 0; k < fftSize / 2; ++k)
    {
        const float db = 20.0f * std::log10 (jmax (fftData[(size_t) k] * 2.0f / (float) fftSize, 1.0e-7f));
        float& s = ch.smoothedDb[(size_t) k];
        s += (db - s) * (db > s ? 0.6f : 0.22f);
    }
}

void SpectrumView::update()
{
    pull (chIn, processor.spectrumIn);
    pull (chOut, processor.spectrumOut);
    if (! hasSignal) return;
    analyse (chIn);
    analyse (chOut);
    repaint();
}

float SpectrumView::xForFreq (float f, float w) const { return std::log (f / 20.0f) / std::log (1000.0f) * w; }
float SpectrumView::freqForX (float x, float w) const { return 20.0f * std::pow (1000.0f, x / w); }

Path SpectrumView::curve (const Channel& ch, Rectangle<float> r, bool closed) const
{
    const double sr = processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.0;
    const float binHz = (float) sr / (float) fftSize;
    const int last = fftSize / 2 - 2;
    Path p;
    bool first = true;

    for (float x = 0; x <= r.getWidth(); x += 2.0f)
    {
        const float b0 = freqForX (x, r.getWidth()) / binHz, b1 = freqForX (x + 2.0f, r.getWidth()) / binHz;
        float v;
        if (b1 - b0 < 1.0f)
        {
            const int i = jmin (last, (int) b0);
            const float fr = jlimit (0.0f, 1.0f, b0 - (float) i);
            v = ch.smoothedDb[(size_t) i] * (1 - fr) + ch.smoothedDb[(size_t) i + 1] * fr;
        }
        else
        {
            v = -200.0f;
            for (int i = (int) b0; i <= jmin (last, (int) std::ceil (b1)); ++i)
                v = jmax (v, ch.smoothedDb[(size_t) i]);
        }
        const float y = r.getY() + (dbTop - jlimit (dbBottom - 10.0f, dbTop + 4.0f, v)) / (dbTop - dbBottom) * r.getHeight();
        if (first) { p.startNewSubPath (r.getX(), y); first = false; }
        else p.lineTo (r.getX() + x, y);
    }

    if (closed)
    {
        p.lineTo (r.getRight(), r.getBottom());
        p.lineTo (r.getX(), r.getBottom());
        p.closeSubPath();
    }
    return p;
}

void SpectrumView::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (col::well);
    g.fillRoundedRectangle (bounds, 7.0f);
    const auto r = bounds.reduced (6.0f);

    Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (col::screen);
    g.fillRect (r);

    const float xo = r.getX() + xForFreq (xover, r.getWidth());
    g.setColour (col::low.withAlpha (0.09f));
    g.fillRect (Rectangle<float> (r.getX(), r.getY(), xo - r.getX(), r.getHeight()));
    g.setColour (col::high.withAlpha (0.06f));
    g.fillRect (Rectangle<float> (xo, r.getY(), r.getRight() - xo, r.getHeight()));

    static const std::pair<float, const char*> freqs[] { { 30.f, "30" }, { 50.f, "50" }, { 100.f, "100" }, { 200.f, "200" }, { 500.f, "500" },
                                                         { 1000.f, "1k" }, { 2000.f, "2k" }, { 5000.f, "5k" }, { 10000.f, "10k" } };
    g.setFont (LeimLookAndFeel::monoFont (10.0f));
    for (auto& [f, label] : freqs)
    {
        const float x = std::round (r.getX() + xForFreq (f, r.getWidth())) + 0.5f;
        g.setColour (col::grid);
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        if (x < r.getRight() - 24)
        {
            g.setColour (col::silkDim.withAlpha (0.6f));
            g.drawText (label, Rectangle<float> (x + 3, r.getBottom() - 15, 40, 12), Justification::centredLeft, false);
        }
    }
    for (float db : { -24.0f, -48.0f, -72.0f })
    {
        const float y = r.getY() + (dbTop - db) / (dbTop - dbBottom) * r.getHeight();
        g.setColour (col::grid);
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        g.setColour (col::silkDim.withAlpha (0.6f));
        g.drawText (minus + String ((int) -db), Rectangle<float> (r.getRight() - 40, y - 14, 36, 12), Justification::centredRight, false);
    }

    if (hasSignal)
    {
        g.setColour (col::silk.withAlpha (0.09f));
        g.fillPath (curve (chIn, r, true));
        g.setColour (col::silkDim.withAlpha (0.45f));
        g.strokePath (curve (chIn, r, false), PathStrokeType (1.0f));

        g.setGradientFill (ColourGradient (col::amber.withAlpha (0.28f), 0, r.getY(), col::amber.withAlpha (0.0f), 0, r.getBottom(), false));
        g.fillPath (curve (chOut, r, true));
        g.setColour (col::amber);
        g.strokePath (curve (chOut, r, false), PathStrokeType (1.6f));
    }
    else
    {
        g.setColour (col::silkDim);
        g.setFont (LeimLookAndFeel::labelFont (12.0f, false));
        g.drawText ("Audio abspielen, um Ein- und Ausgangsspektrum zu sehen", r.reduced (20), Justification::centred, true);
    }

    // Crossover
    g.setColour (col::silk.withAlpha (0.85f));
    g.fillRect (std::round (xo) - 0.75f, r.getY(), 1.5f, r.getHeight());
    const auto box = Rectangle<float> (jlimit (r.getX() + 2, r.getRight() - 58, xo - 28), r.getBottom() - 38, 56, 16);
    g.setColour (col::silk);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (col::ink);
    g.setFont (LeimLookAndFeel::monoFont (10.5f));
    g.drawText (String (roundToInt (xover)) + " Hz", box, Justification::centred, false);

    g.setFont (LeimLookAndFeel::labelFont (10.5f));
    g.setColour (col::low);
    g.drawText ("LOW", Rectangle<float> (xo - 46, r.getY() + 6, 40, 14), Justification::centredRight, false);
    g.setColour (col::high);
    g.drawText ("HIGH", Rectangle<float> (xo + 6, r.getY() + 6, 40, 14), Justification::centredLeft, false);

    // Legende
    g.setFont (LeimLookAndFeel::monoFont (10.0f));
    const float ly = r.getY() + 13;
    g.setColour (col::silkDim.withAlpha (0.6f));
    g.fillRect (r.getRight() - 150, ly - 1, 12.0f, 2.0f);
    g.setColour (col::silkDim);
    g.drawText ("Eingang", Rectangle<float> (r.getRight() - 134, ly - 7, 60, 14), Justification::centredLeft, false);
    g.setColour (col::amber);
    g.fillRect (r.getRight() - 76, ly - 1, 12.0f, 2.0f);
    g.setColour (col::silkDim);
    g.drawText ("Ausgang", Rectangle<float> (r.getRight() - 60, ly - 7, 60, 14), Justification::centredLeft, false);
}

void SpectrumView::setXoverFromMouse (const MouseEvent& e)
{
    const auto r = getLocalBounds().toFloat().reduced (6.0f);
    const float f = freqForX (jlimit (0.0f, r.getWidth(), e.position.x - r.getX()), r.getWidth());
    xoverAttachment.setValueAsPartOfGesture ((float) roundToInt (jlimit (40.0f, 400.0f, f)));
}

void SpectrumView::mouseDown (const MouseEvent& e) { xoverAttachment.beginGesture(); setXoverFromMouse (e); }
void SpectrumView::mouseDrag (const MouseEvent& e) { setXoverFromMouse (e); }
void SpectrumView::mouseUp (const MouseEvent&)     { xoverAttachment.endGesture(); }
void SpectrumView::mouseMove (const MouseEvent&)   {}

//==============================================================================
void CurveView::setShape (int s, float d, float l, bool a)
{
    if (valid && s == style && exactlyEqual (d, drive) && exactlyEqual (l, level) && a == autoGain) return;
    style = s; drive = d; level = l; autoGain = a; valid = true;
    repaint();
}

void CurveView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::screen);
    g.fillRoundedRectangle (r, 3.0f);

    const float W = r.getWidth(), H = r.getHeight();
    g.setColour (col::silkDim.withAlpha (0.22f));
    const float dash[] { 2.0f, 3.0f };
    g.drawDashedLine ({ r.getX(), r.getBottom(), r.getRight(), r.getY() }, dash, 2, 1.0f);
    g.drawDashedLine ({ r.getX(), r.getCentreY(), r.getRight(), r.getCentreY() }, dash, 2, 1.0f);
    g.drawDashedLine ({ r.getCentreX(), r.getY(), r.getCentreX(), r.getBottom() }, dash, 2, 1.0f);

    const double gd = leim::dbToGain (drive);
    const double comp = (autoGain ? leim::gainCompensation (style, gd) : 1.0) * leim::dbToGain (level);

    Path p;
    for (int i = 0; i <= (int) W; ++i)
    {
        const double x = (double) i / W * 2.0 - 1.0;
        const double y = jlimit (-1.1, 1.1, leim::shape (style, gd * x) * comp);
        const float py = r.getCentreY() - (float) y * H * 0.5f * 0.9f;
        if (i == 0) p.startNewSubPath (r.getX(), py); else p.lineTo (r.getX() + (float) i, py);
    }
    g.setColour (tone);
    g.strokePath (p, PathStrokeType (1.6f));
}
