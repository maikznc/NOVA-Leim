#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Presets.h"
#include "dsp/LeimDSP.h"

// Lock-freier Abgriff für die Spektrumanzeige (Audio-Thread schreibt, GUI liest)
class SpectrumFifo
{
public:
    static constexpr int capacity = 32768;

    void push (const float* l, const float* r, int n) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) data[(size_t) (s1 + i)] = 0.5f * (l[i] + r[i]);
        for (int i = 0; i < z2; ++i) data[(size_t) (s2 + i)] = 0.5f * (l[z1 + i] + r[z1 + i]);
        fifo.finishedWrite (z1 + z2);
    }

    int pull (float* dest, int maxSamples) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToRead (maxSamples, s1, z1, s2, z2);
        std::copy_n (data.begin() + s1, z1, dest);
        std::copy_n (data.begin() + s2, z2, dest + z1);
        fifo.finishedRead (z1 + z2);
        return z1 + z2;
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::vector<float> data = std::vector<float> ((size_t) capacity);
};

class LeimAudioProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    LeimAudioProcessor();
    ~LeimAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void applyPreset (int globalIndex);
    bool isPresetModified() const noexcept { return presetDirty.load(); }

    juce::AudioProcessorValueTreeState apvts;

    // Messwerte für die Oberfläche (GUI liest per exchange)
    std::atomic<float> meterInPeak { 0 }, meterOutPeak { 0 }, meterGr { 0 };
    std::atomic<float> meterInRms { 0 }, meterOutRms { 0 }, meterDryRms { 0 };
    std::atomic<juce::uint32> blockCounter { 0 };
    SpectrumFifo spectrumIn, spectrumOut;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    static void storeMax (std::atomic<float>& a, float v) noexcept;

    ParamReader reader;
    leim::LeimDSP dsp;
    leim::Params lastParams;
    bool paramsInitialised = false;

    std::atomic<int> currentProgram { 0 };
    std::atomic<bool> presetDirty { false }, applyingPreset { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LeimAudioProcessor)
};
