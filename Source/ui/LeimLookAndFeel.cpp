#include "LeimLookAndFeel.h"

using namespace juce;

LeimLookAndFeel::LeimLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, col::room);
    setColour (ComboBox::backgroundColourId, col::room2);
    setColour (ComboBox::textColourId, col::silk);
    setColour (ComboBox::outlineColourId, col::line);
    setColour (ComboBox::arrowColourId, col::silkDim);
    setColour (PopupMenu::backgroundColourId, col::room2);
    setColour (PopupMenu::textColourId, col::silk);
    setColour (PopupMenu::highlightedBackgroundColourId, col::amber);
    setColour (PopupMenu::highlightedTextColourId, col::ink);
    setColour (PopupMenu::headerTextColourId, col::silkDim);
    setColour (Label::textColourId, col::silk);
    setColour (TextButton::textColourOffId, col::silk);
    setColour (TextButton::textColourOnId, col::ink);
    setColour (Slider::textBoxTextColourId, col::amberHot);
    setColour (Slider::textBoxBackgroundColourId, col::screen);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (TooltipWindow::backgroundColourId, col::room2);
    setColour (TooltipWindow::textColourId, col::silk);
}

Font LeimLookAndFeel::labelFont (float height, bool bold)
{
    Font f (FontOptions (height, bold ? Font::bold : Font::plain));
    f.setExtraKerningFactor (0.08f);
    return f;
}

Font LeimLookAndFeel::monoFont (float height)
{
    return Font (FontOptions (Font::getDefaultMonospacedFontName(), height, Font::plain));
}

Font LeimLookAndFeel::brandFont (float height)
{
    Font f (FontOptions (height, Font::bold));
    f.setExtraKerningFactor (0.3f);
    return f;
}

//==============================================================================
void LeimLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r  = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 1.0f;
    const auto  c  = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto  tone  = s.findColour (Slider::rotarySliderFillColourId);

    // Skalenstriche
    int ticks = 11;
    if (s.getInterval() >= 1.0 && s.getMaximum() - s.getMinimum() <= 12.0)
        ticks = (int) (s.getMaximum() - s.getMinimum()) + 1;

    g.setColour (col::silkDim);
    for (int i = 0; i < ticks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (ticks - 1);
        const auto p0 = c.getPointOnCircumference (r * 0.92f, a);
        const auto p1 = c.getPointOnCircumference (r * 1.00f, a);
        g.drawLine ({ p0, p1 }, 1.4f);
    }

    // Spur und Wert-Bogen
    const float arcR = r * 0.80f, thick = jmax (3.0f, r * 0.11f);
    const PathStrokeType stroke (thick, PathStrokeType::curved, PathStrokeType::rounded);

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::black.withAlpha (0.38f));
    g.strokePath (track, stroke);

    float from = startAngle;
    if ((bool) s.getProperties().getWithDefault ("bipolar", false))
        from = startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle);

    if (std::abs (angle - from) > 0.01f)
    {
        Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (tone);
        g.strokePath (value, stroke);
    }

    // Bakelit-Kappe
    const float capR = r * 0.62f;
    Path cap;
    cap.addEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
    DropShadow (Colours::black.withAlpha (0.55f), (int) (r * 0.18f), { 0, (int) (r * 0.08f) }).drawForPath (g, cap);

    g.setGradientFill (ColourGradient (col::knobHi, c.x - capR * 0.35f, c.y - capR * 0.45f,
                                       Colour (0xff0c0e11), c.x + capR, c.y + capR, true));
    g.fillPath (cap);
    g.setColour (col::edge);
    g.strokePath (cap, PathStrokeType (1.0f));
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - capR + 1.5f, c.y - capR + 1.5f, capR * 2 - 3.0f, capR * 2 - 3.0f, 1.0f);

    // Zeiger
    Path ptr;
    const float pw = jmax (2.0f, r * 0.065f);
    ptr.addRoundedRectangle (-pw * 0.5f, -capR * 0.92f, pw, capR * 0.58f, pw * 0.5f);
    g.setColour (s.isEnabled() ? col::silk : col::silkDim);
    g.fillPath (ptr, AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
void LeimLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const auto& props = b.getProperties();
    const bool flat = (bool) props.getWithDefault ("flat", false);       // Taste in einer Mulde
    const Colour tone ((uint32) (int64) props.getWithDefault ("tone", 0));

    if (on)
    {
        if (! tone.isTransparent())
        {
            g.setColour (tone.withAlpha (0.35f));
            g.fillRoundedRectangle (r.expanded (1.5f), 4.0f);
            g.setColour (tone);
        }
        else
        {
            g.setGradientFill (ColourGradient (col::keyHi, 0, r.getY(), col::keyLo, 0, r.getBottom(), false));
        }
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
        return;
    }

    if (flat)
    {
        if (highlighted) { g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 3.0f); }
        return;
    }

    g.setGradientFill (ColourGradient (down ? col::plateLo : col::plateHi, 0, r.getY(),
                                       down ? col::plateHi : col::plateLo, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (highlighted ? 0.2f : 0.13f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
}

void LeimLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    const bool mono = (bool) b.getProperties().getWithDefault ("mono", false);
    const bool flat = (bool) b.getProperties().getWithDefault ("flat", false);
    g.setFont (mono ? monoFont (13.0f) : labelFont (11.5f));
    g.setColour (on ? col::ink : (flat ? col::silkDim : col::silk));
    g.drawText (mono ? b.getButtonText() : b.getButtonText().toUpperCase(), b.getLocalBounds(), Justification::centred, false);
}

void LeimLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool dark = (bool) b.getProperties().getWithDefault ("dark", false);
    const bool redLamp = (bool) b.getProperties().getWithDefault ("red", false);

    if (dark)
    {
        g.setColour (col::room2);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (highlighted ? col::silkDim : col::line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
    else
    {
        g.setGradientFill (ColourGradient (down ? col::plateLo : col::plateHi, 0, r.getY(),
                                           down ? col::plateHi : col::plateLo, 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (col::edge);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (Colours::white.withAlpha (0.14f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
    }

    const auto lampC = Point<float> (r.getX() + 14.0f, r.getCentreY());
    const bool on = b.getToggleState();
    const Colour lit = redLamp ? Colour (0xffff8a7c) : col::amberHot;
    const Colour glow = redLamp ? col::red : col::amber;

    if (on)
    {
        g.setGradientFill (ColourGradient (glow.withAlpha (0.55f), lampC.x, lampC.y, glow.withAlpha (0.0f), lampC.x + 9, lampC.y, true));
        g.fillEllipse (lampC.x - 9, lampC.y - 9, 18, 18);
    }
    g.setColour (on ? lit : Colour (0xff3b2c1d));
    g.fillEllipse (lampC.x - 4, lampC.y - 4, 8, 8);

    g.setColour (col::silk);
    g.setFont (labelFont (11.5f));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (26.0f).withTrimmedRight (6.0f), Justification::centredLeft, false);
}

//==============================================================================
void LeimLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::room2);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? col::amberHot : col::line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.startNewSubPath (ax - 4, ay - 2);
    arrow.lineTo (ax, ay + 2);
    arrow.lineTo (ax + 4, ay - 2);
    g.setColour (col::silkDim);
    g.strokePath (arrow, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

Font LeimLookAndFeel::getComboBoxFont (ComboBox&) { return labelFont (13.0f); }
Font LeimLookAndFeel::getPopupMenuFont()          { return labelFont (14.0f, false); }

void LeimLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void LeimLookAndFeel::drawLabel (Graphics& g, Label& label)
{
    const auto bg = label.findColour (Label::backgroundColourId);
    if (! bg.isTransparent())
    {
        g.setColour (bg);
        g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 3.0f);
    }

    if (! label.isBeingEdited())
    {
        g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.5f));
        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(), label.getBorderSize().subtractedFrom (label.getLocalBounds()),
                          label.getJustificationType(), 1, label.getMinimumHorizontalScale());
    }
}
