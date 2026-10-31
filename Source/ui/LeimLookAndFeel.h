#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace col
{
    inline const juce::Colour room     { 0xff13171c }, room2   { 0xff1c2229 }, line    { 0xff2c343d };
    inline const juce::Colour plate    { 0xff59636e }, plateHi { 0xff69747f }, plateLo { 0xff4a535d };
    inline const juce::Colour seam     { 0xff3a424b }, edge    { 0xff0b0d10 }, well    { 0xff252b32 };
    inline const juce::Colour screen   { 0xff0d1114 }, grid    { 0xff1d2730 };
    inline const juce::Colour silk     { 0xffefe6d2 }, silkDim { 0xffc4bdac }, ink     { 0xff1b1d20 };
    inline const juce::Colour keyHi    { 0xffece2ca }, keyLo   { 0xffcbbd9c };
    inline const juce::Colour amber    { 0xfff0a43a }, amberHot{ 0xffffd389 };
    inline const juce::Colour vu       { 0xffefdcae }, vuHot   { 0xfffff3d4 };
    inline const juce::Colour low      { 0xffef7f47 }, high    { 0xff7dbad3 }, red     { 0xffe64a3d };
    inline const juce::Colour knob     { 0xff1a1e23 }, knobHi  { 0xff4a535d };
}

class LeimLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LeimLookAndFeel();

    static juce::Font labelFont (float height, bool bold = true);
    static juce::Font monoFont  (float height);
    static juce::Font brandFont (float height);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    void drawLabel (juce::Graphics&, juce::Label&) override;
};
