#pragma once

#include "PluginProcessor.h"
#include "ui/LeimLookAndFeel.h"
#include "ui/Widgets.h"

class LeimEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    static constexpr int baseWidth = 1120, baseHeight = 720;

    explicit LeimEditor (LeimAudioProcessor&);
    ~LeimEditor() override;

    void resized() override;

private:
    void timerCallback() override;

    class Content;
    LeimLookAndFeel lookAndFeel;
    std::unique_ptr<Content> content;
    double lastTick = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LeimEditor)
};
