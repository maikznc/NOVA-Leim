#include "PluginEditor.h"

using namespace juce;

namespace
{
    const String minus = String::fromUTF8 ("\xe2\x88\x92");

    // Halbtransparente Abdeckung, wenn Bypass aktiv ist
    class Dimmer : public Component
    {
    public:
        Dimmer() { setInterceptsMouseClicks (false, false); }
        void setActive (bool a) { if (a != active) { active = a; repaint(); } }
        void paint (Graphics& g) override
        {
            if (! active) return;
            g.setColour (Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        }
    private:
        bool active = false;
    };

    void place (Knob& k, int centreX, int y)
    {
        k.setBounds (centreX - k.preferredWidth() / 2, y, k.preferredWidth(), k.preferredHeight());
    }

    Font titleFont (float h)
    {
        auto f = LeimLookAndFeel::brandFont (h);
        f.setExtraKerningFactor (0.2f);
        return f;
    }
}

//==============================================================================
class LeimEditor::Content : public Component
{
public:
    explicit Content (LeimAudioProcessor& p) : proc (p), state (p.apvts)
    {
        // Preset-Leiste
        const auto& genres = leim::presetGenres();
        for (int i = 0; i < (int) genres.size(); ++i)
            genreBox.addItem (U (genres[(size_t) i].name), i + 1);

        genreBox.onChange = [this]
        {
            const int g = genreBox.getSelectedId() - 1;
            if (g >= 0) loadPreset (leim::globalPresetIndex (g, 0));
        };
        presetBox.onChange = [this]
        {
            const int i = presetBox.getSelectedId() - 1;
            if (i >= 0 && shownGenre >= 0) loadPreset (leim::globalPresetIndex (shownGenre, i));
        };
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (+1); };
        prevBtn.getProperties().set ("mono", true);
        nextBtn.getProperties().set ("mono", true);
        prevBtn.setTooltip (U ("Vorheriges Preset"));
        nextBtn.setTooltip (U ("Nächstes Preset"));

        countLbl.setFont (LeimLookAndFeel::monoFont (12.0f));
        countLbl.setColour (Label::textColourId, col::silkDim);

        deltaVal.setFont (LeimLookAndFeel::monoFont (14.0f));
        deltaVal.setColour (Label::textColourId, col::amberHot);
        deltaVal.setColour (Label::backgroundColourId, col::screen);
        deltaVal.setJustificationType (Justification::centred);
        deltaVal.setText (U ("–"), dontSendNotification);

        for (Component* c : std::initializer_list<Component*> {
                 &genreBox, &presetBox, &prevBtn, &nextBtn, &countLbl, &orderBtn, &bypassBtn,
                 &grMeter, &threshold, &makeup, &ratio, &attack, &release, &range, &schpf, &cmix,
                 &spectrum, &loStyles, &hiStyles, &loDrive, &loLevel, &hiDrive, &hiLevel, &loCurve, &hiCurve,
                 &xover, &smix, &autoGain, &osBtns,
                 &meters, &deltaVal, &inGain, &outGain, &clipBtn, &dimmer })
            addAndMakeVisible (c);

        order = paramIndex (ids::order);
        refreshPresetUi();
    }

    void tick (double dt)
    {
        // Gain Reduction: nur neu lesen, wenn seit dem letzten Frame Audio verarbeitet wurde
        const auto blocks = proc.blockCounter.load();
        if (blocks != lastBlocks) { lastBlocks = blocks; grTarget = proc.meterGr.exchange (0.0f); }

        const bool bypassed = paramValue (ids::bypass) > 0.5f;
        grMeter.update (bypassed ? 0.0f : grTarget, dt);
        meters.update (proc.meterInPeak.exchange (0.0f), proc.meterOutPeak.exchange (0.0f), dt);

        const float k = 1.0f - std::exp (-(float) dt / 0.5f);
        rmsOut += (proc.meterOutRms.load() - rmsOut) * k;
        rmsDry += (proc.meterDryRms.load() - rmsDry) * k;

        deltaClock += dt;
        if (deltaClock > 0.16)
        {
            deltaClock = 0;
            if (rmsDry > 1e-4f && rmsOut > 1e-4f)
            {
                const float d = 20.0f * std::log10 (rmsOut / rmsDry);
                deltaVal.setText ((d > 0.05f ? "+" : d < -0.05f ? minus : String()) + String (std::abs (d), 1).replaceCharacter ('.', ',') + " dB",
                                  dontSendNotification);
            }
            else deltaVal.setText (U ("–"), dontSendNotification);
        }

        if ((spectrumFrame ^= 1) != 0)
            spectrum.update();

        loCurve.setShape (paramIndex (ids::loStyle), paramValue (ids::loDrive), paramValue (ids::loLevel), paramValue (ids::autoGain) > 0.5f);
        hiCurve.setShape (paramIndex (ids::hiStyle), paramValue (ids::hiDrive), paramValue (ids::hiLevel), paramValue (ids::autoGain) > 0.5f);

        const int xo = roundToInt (paramValue (ids::xover));
        if (xo != shownXover) { shownXover = xo; repaint (loBandR); repaint (hiBandR); }

        const int os = paramIndex (ids::oversampling);
        if (os != shownOs) { shownOs = os; repaint (satR.withHeight (40)); repaint (brand); }

        const int o = paramIndex (ids::order);
        if (o != order) { order = o; layoutModules(); repaint(); }

        if (bypassed != shownBypass) { shownBypass = bypassed; dimmer.setActive (bypassed); repaint (brand); }

        refreshPresetUi();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (col::room);

        g.setColour (col::silkDim);
        g.setFont (LeimLookAndFeel::labelFont (12.0f));
        g.drawText ("PRESET", Rectangle<int> (16, 12, 60, 32), Justification::centredLeft, false);

        paintPlate (g);
        paintBrand (g);

        paintModule (g, compR, "KOMPRESSOR", U ("VCA  ·  BUS GLUE"));
        paintModule (g, satR, U ("SÄTTIGUNG"), U ("2-BAND  ·  ") + String (osFactor()) + U ("× OVERSAMPLING"));
        paintModule (g, outR, "AUSGANG", {});

        paintBand (g, loBandR, "LOW",  col::low,  "bis " + String (shownXover) + " Hz");
        paintBand (g, hiBandR, "HIGH", col::high, "ab "  + String (shownXover) + " Hz");

        g.setColour (col::silk);
        g.setFont (LeimLookAndFeel::labelFont (11.5f));
        g.drawText ("RATIO", ratio.getBounds().withY (ratio.getY() - 20).withHeight (16).expanded (10, 0), Justification::centred, false);
        g.drawText ("OVERSAMPLING", osBtns.getBounds().withY (osBtns.getY() - 20).withHeight (16).withWidth (140), Justification::centredLeft, false);
        g.drawText (U ("Δ ZUM ORIGINAL"), deltaVal.getBounds().withY (deltaVal.getY() - 20).withHeight (16).expanded (20, 0), Justification::centred, false);
    }

    void resized() override
    {
        int x = 80;
        genreBox.setBounds (x, 12, 200, 32);   x += 208;
        presetBox.setBounds (x, 12, 230, 32);  x += 238;
        prevBtn.setBounds (x, 12, 32, 32);     x += 36;
        nextBtn.setBounds (x, 12, 32, 32);     x += 42;
        countLbl.setBounds (x, 12, 70, 32);

        bypassBtn.setBounds (getWidth() - 16 - 112, 12, 112, 32);
        orderBtn.setBounds (bypassBtn.getX() - 10 - 146, 12, 146, 32);

        plate = { 16, 60, getWidth() - 32, getHeight() - 76 };
        brand = { plate.getX() + 24, plate.getY() + 12, plate.getWidth() - 48, 36 };
        layoutModules();
    }

private:
    //==========================================================================
    float paramValue (const char* id) const { return state.getRawParameterValue (id)->load(); }
    int paramIndex (const char* id) const   { return roundToInt (paramValue (id)); }
    int osFactor() const                    { return choices::oversampling[(size_t) jlimit (0, 2, paramIndex (ids::oversampling))]; }

    void loadPreset (int index)
    {
        proc.applyPreset (index);
        proc.updateHostDisplay (AudioProcessor::ChangeDetails().withProgramChanged (true));
        refreshPresetUi();
    }

    void stepPreset (int dir)
    {
        const int n = leim::totalPresetCount();
        loadPreset ((proc.getCurrentProgram() + dir + n) % n);
    }

    void refreshPresetUi()
    {
        const int prog = proc.getCurrentProgram();
        const bool modified = proc.isPresetModified();
        if (prog == shownProgram && modified == shownModified) return;

        int g = 0, p = 0;
        if (! leim::locatePreset (prog, g, p)) return;
        const auto& genre = leim::presetGenres()[(size_t) g];

        if (g != shownGenre)
        {
            presetBox.clear (dontSendNotification);
            for (int i = 0; i < (int) genre.presets.size(); ++i)
                presetBox.addItem (U (genre.presets[(size_t) i].name), i + 1);
            shownGenre = g;
        }

        genreBox.setSelectedId (g + 1, dontSendNotification);
        if (modified) presetBox.setText (U ("Eigene Einstellung"), dontSendNotification);
        else          presetBox.setSelectedId (p + 1, dontSendNotification);
        countLbl.setText (String (p + 1) + "/" + String ((int) genre.presets.size()), dontSendNotification);

        shownProgram = prog;
        shownModified = modified;
    }

    //==========================================================================
    void layoutModules()
    {
        const bool swapped = order == 1;
        const int top = plate.getY() + 60, h = plate.getBottom() - 18 - top;
        const int x = plate.getX() + 20, compW = 372, satW = 490, outW = 158, gap = 14;

        compR = { swapped ? x + satW + gap : x, top, compW, h };
        satR  = { swapped ? x : x + compW + gap, top, satW, h };
        outR  = { x + compW + satW + 2 * gap, top, outW, h };
        dimmer.setBounds (compR.getUnion (satR).getUnion (outR));

        layoutComp();
        layoutSat();
        layoutOut();
    }

    void layoutComp()
    {
        const auto r = compR;
        const int ix = r.getX() + 14, iw = r.getWidth() - 28;
        grMeter.setBounds (ix, r.getY() + 46, iw, 172);

        const int rowY = r.getY() + 236;
        place (threshold, ix + iw / 6, rowY);
        place (makeup, ix + iw * 5 / 6, rowY);
        ratio.setBounds (ix + iw / 2 - 26, rowY + 20, 52, 96);

        const int smallY = r.getY() + 384;
        Knob* small[] { &attack, &release, &range, &schpf, &cmix };
        for (int i = 0; i < 5; ++i)
            place (*small[i], ix + (int) ((i + 0.5f) * (float) iw / 5.0f), smallY);
    }

    void layoutBand (Rectangle<int> b, ChoiceButtons& styles, Knob& drive, Knob& level, CurveView& curve)
    {
        styles.setBounds (b.getX() + 10, b.getY() + 34, b.getWidth() - 20, 56);
        const int ky = b.getY() + 100;
        place (drive, b.getX() + 62, ky);
        const int sideX = b.getRight() - 50;
        curve.setBounds (sideX - 38, ky, 76, 46);
        place (level, sideX, ky + 52);
    }

    void layoutSat()
    {
        const auto r = satR;
        const int ix = r.getX() + 14, iw = r.getWidth() - 28;
        spectrum.setBounds (ix, r.getY() + 46, iw, 146);

        const int by = r.getY() + 202, bw = (iw - 10) / 2;
        loBandR = { ix, by, bw, 244 };
        hiBandR = { ix + bw + 10, by, bw, 244 };
        layoutBand (loBandR, loStyles, loDrive, loLevel, loCurve);
        layoutBand (hiBandR, hiStyles, hiDrive, hiLevel, hiCurve);

        const int rowY = by + 244 + 12;
        place (xover, ix + 60, rowY);
        place (smix, ix + 172, rowY);
        autoGain.setBounds (ix + iw - 172, rowY + 2, 172, 32);
        osBtns.setBounds (ix + iw - 172, rowY + 60, 132, 32);
    }

    void layoutOut()
    {
        const auto r = outR;
        const int ix = r.getX() + 14, iw = r.getWidth() - 28;
        meters.setBounds (ix, r.getY() + 46, iw, 232);
        deltaVal.setBounds (ix + iw / 2 - 50, r.getY() + 308, 100, 26);
        place (inGain, ix + iw / 4, r.getY() + 350);
        place (outGain, ix + iw * 3 / 4, r.getY() + 350);
        clipBtn.setBounds (ix + iw / 2 - 62, r.getY() + 470, 124, 32);
    }

    //==========================================================================
    void paintPlate (Graphics& g)
    {
        const auto pr = plate.toFloat();
        DropShadow (Colours::black.withAlpha (0.6f), 34, { 0, 18 }).drawForRectangle (g, plate);

        ColourGradient grad (col::plateHi, 0, pr.getY(), col::plateLo, 0, pr.getBottom(), false);
        grad.addColour (0.16, col::plate);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (pr, 10.0f);

        {
            Graphics::ScopedSaveState s (g);
            Path clip;
            clip.addRoundedRectangle (pr, 10.0f);
            g.reduceClipRegion (clip);
            g.setColour (Colours::white.withAlpha (0.022f));
            for (int x = plate.getX(); x < plate.getRight(); x += 3)
                g.fillRect (x, plate.getY(), 1, plate.getHeight());
        }

        g.setColour (col::edge);
        g.drawRoundedRectangle (pr.expanded (0.5f), 10.0f, 1.0f);
        g.setColour (Colours::white.withAlpha (0.2f));
        g.drawHorizontalLine (plate.getY() + 1, pr.getX() + 10, pr.getRight() - 10);

        for (auto c : { Point<float> (pr.getX() + 13, pr.getY() + 13), Point<float> (pr.getRight() - 13, pr.getY() + 13),
                        Point<float> (pr.getX() + 13, pr.getBottom() - 13), Point<float> (pr.getRight() - 13, pr.getBottom() - 13) })
        {
            g.setGradientFill (ColourGradient (Colour (0xffb9c0c7), c.x - 2, c.y - 2, Colour (0xff3d444b), c.x + 5, c.y + 5, true));
            g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11, 11);
            g.setColour (Colour (0xff30363c));
            const auto d = Point<float> (std::cos (-0.49f), std::sin (-0.49f)) * 4.0f;
            g.drawLine ({ c - d, c + d }, 1.5f);
        }
    }

    void paintBrand (Graphics& g)
    {
        const auto b = brand;
        g.setColour (col::amberHot);
        g.setFont (LeimLookAndFeel::brandFont (13.0f));
        g.drawText ("NOVA", Rectangle<int> (b.getX(), b.getY(), 74, b.getHeight()), Justification::centredLeft, false);
        g.setColour (col::silk.withAlpha (0.35f));
        g.fillRect (b.getX() + 80, b.getY() + 6, 1, b.getHeight() - 12);

        g.setColour (col::silk);
        g.setFont (LeimLookAndFeel::brandFont (27.0f));
        g.drawText ("LEIM", Rectangle<int> (b.getX() + 96, b.getY(), 140, b.getHeight()), Justification::centredLeft, false);

        const auto model = Rectangle<float> ((float) b.getX() + 240, (float) b.getCentreY() - 10, 44, 20);
        g.setColour (col::silkDim);
        g.drawRoundedRectangle (model, 3.0f, 1.0f);
        g.setColour (col::silk);
        g.setFont (titleFont (10.0f));
        g.drawText ("LB-2", model, Justification::centred, false);

        g.setColour (col::silkDim);
        g.setFont (LeimLookAndFeel::labelFont (12.5f));
        g.drawText (String::fromUTF8 ("LOW-END BUS  \xc2\xb7  GLUE + DRIVE"), Rectangle<int> (b.getX() + 298, b.getY(), 300, b.getHeight()),
                    Justification::centredLeft, false);

        const double sr = proc.getSampleRate();
        String status = sr > 0 ? String (sr / 1000.0, 1).replaceCharacter ('.', ',') + " kHz" : String ("Bereit");
        if (shownBypass) status += String::fromUTF8 ("  \xc2\xb7  Bypass");
        g.setFont (LeimLookAndFeel::monoFont (12.0f));
        g.drawText (status, Rectangle<int> (b.getRight() - 260, b.getY(), 260, b.getHeight()), Justification::centredRight, false);
    }

    void paintModule (Graphics& g, Rectangle<int> r, const String& title, const String& sub)
    {
        const auto rf = r.toFloat();
        g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.12f), 0, rf.getY(), Colours::transparentBlack, 0, rf.getY() + rf.getHeight() * 0.6f, false));
        g.fillRoundedRectangle (rf, 6.0f);
        g.setColour (col::seam);
        g.drawRoundedRectangle (rf.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (r.getY() + 1, rf.getX() + 6, rf.getRight() - 6);

        const auto head = Rectangle<int> (r.getX() + 14, r.getY() + 10, r.getWidth() - 28, 18);
        g.setColour (col::silk);
        g.setFont (titleFont (11.5f));
        g.drawText (title, head, Justification::centredLeft, false);
        g.setColour (col::silkDim);
        g.setFont (LeimLookAndFeel::labelFont (11.0f));
        g.drawText (sub, head, Justification::centredRight, false);
        g.setColour (col::silk.withAlpha (0.28f));
        g.fillRect (r.getX() + 14, r.getY() + 34, r.getWidth() - 28, 1);
    }

    void paintBand (Graphics& g, Rectangle<int> r, const String& name, Colour tone, const String& rangeText)
    {
        const auto rf = r.toFloat();
        g.setColour (Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (rf, 5.0f);
        g.setColour (col::seam);
        g.drawRoundedRectangle (rf.reduced (0.5f), 5.0f, 1.0f);

        const float dotX = rf.getX() + 14, dotY = rf.getY() + 18;
        g.setGradientFill (ColourGradient (tone.withAlpha (0.5f), dotX, dotY, tone.withAlpha (0.0f), dotX + 8, dotY, true));
        g.fillEllipse (dotX - 8, dotY - 8, 16, 16);
        g.setColour (tone);
        g.fillEllipse (dotX - 3.5f, dotY - 3.5f, 7, 7);

        g.setFont (titleFont (10.5f));
        g.drawText (name, Rectangle<int> (r.getX() + 24, r.getY() + 10, 80, 16), Justification::centredLeft, false);
        g.setColour (col::silkDim);
        g.setFont (LeimLookAndFeel::monoFont (11.0f));
        g.drawText (rangeText, Rectangle<int> (r.getRight() - 120, r.getY() + 10, 110, 16), Justification::centredRight, false);
    }

    //==========================================================================
    LeimAudioProcessor& proc;
    AudioProcessorValueTreeState& state;

    ComboBox genreBox, presetBox;
    TextButton prevBtn { U ("‹") }, nextBtn { U ("›") };
    Label countLbl;
    ChoiceToggle orderBtn { *state.getParameter (ids::order), StringArray (U ("Komp → Sat"), U ("Sat → Komp")), true };
    LampButton bypassBtn { state, ids::bypass, "Bypass", true, true };

    GainReductionMeter grMeter;
    Knob threshold { state, ids::threshold, "Threshold", Knob::Large, col::amber };
    Knob makeup    { state, ids::makeup, "Makeup", Knob::Large, col::amber };
    ChoiceButtons ratio { *state.getParameter (ids::ratio), StringArray { "2", "4", "10" }, 1, Colours::transparentBlack, true, true };
    Knob attack    { state, ids::attack, "Attack", Knob::Medium, col::amber };
    Knob release   { state, ids::release, "Release", Knob::Medium, col::amber };
    Knob range     { state, ids::range, "Range", Knob::Medium, col::amber };
    Knob schpf     { state, ids::scHpf, "SC-HPF", Knob::Medium, col::amber };
    Knob cmix      { state, ids::compMix, "Mix", Knob::Medium, col::amber };

    SpectrumView spectrum { proc };
    ChoiceButtons loStyles { *state.getParameter (ids::loStyle), choices::styleNames(), 3, col::low };
    ChoiceButtons hiStyles { *state.getParameter (ids::hiStyle), choices::styleNames(), 3, col::high };
    Knob loDrive { state, ids::loDrive, "Drive", Knob::Large, col::low };
    Knob loLevel { state, ids::loLevel, "Level", Knob::Small, col::low, true };
    Knob hiDrive { state, ids::hiDrive, "Drive", Knob::Large, col::high };
    Knob hiLevel { state, ids::hiLevel, "Level", Knob::Small, col::high, true };
    CurveView loCurve { col::low }, hiCurve { col::high };
    Knob xover { state, ids::xover, "Crossover", Knob::Medium, col::amber };
    Knob smix  { state, ids::satMix, "Mix", Knob::Medium, col::amber };
    LampButton autoGain { state, ids::autoGain, "Pegelausgleich" };
    ChoiceButtons osBtns { *state.getParameter (ids::oversampling), StringArray (U ("1×"), U ("2×"), U ("4×")), 3, Colours::transparentBlack, true, true };

    LevelMeters meters;
    Label deltaVal;
    Knob inGain  { state, ids::inGain, "Input", Knob::Medium, col::silk, true };
    Knob outGain { state, ids::outGain, "Output", Knob::Medium, col::silk, true };
    LampButton clipBtn { state, ids::softClip, "Soft Clip" };

    Dimmer dimmer;

    Rectangle<int> plate, brand, compR, satR, outR, loBandR, hiBandR;
    int order = 0, shownGenre = -1, shownProgram = -1, shownXover = 120, shownOs = -1;
    bool shownModified = false, shownBypass = false;
    uint32 lastBlocks = 0;
    float grTarget = 0, rmsOut = 0, rmsDry = 0;
    double deltaClock = 0;
    int spectrumFrame = 0;
};

//==============================================================================
LeimEditor::LeimEditor (LeimAudioProcessor& p)
    : AudioProcessorEditor (p)
{
    content = std::make_unique<Content> (p);
    content->setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (*content);
    content->setBounds (0, 0, baseWidth, baseHeight);

    setResizable (true, true);
    setResizeLimits (baseWidth * 6 / 10, baseHeight * 6 / 10, baseWidth * 3 / 2, baseHeight * 3 / 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (baseWidth, baseHeight);

    lastTick = Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

LeimEditor::~LeimEditor()
{
    stopTimer();
    content->setLookAndFeel (nullptr);
}

void LeimEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content->setTransform (AffineTransform::scale (scale));
}

void LeimEditor::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const double dt = jlimit (0.001, 0.05, (now - lastTick) / 1000.0);
    lastTick = now;
    content->tick (dt);
}
