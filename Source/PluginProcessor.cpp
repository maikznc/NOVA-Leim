#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const char* const kAllIds[] {
        ids::threshold, ids::ratio, ids::attack, ids::release, ids::makeup, ids::range, ids::scHpf, ids::compMix,
        ids::xover, ids::loStyle, ids::loDrive, ids::loLevel, ids::hiStyle, ids::hiDrive, ids::hiLevel, ids::satMix,
        ids::autoGain, ids::oversampling, ids::inGain, ids::outGain, ids::softClip, ids::order, ids::bypass
    };
}

LeimAudioProcessor::LeimAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LEIM", createParameterLayout()),
      reader (apvts)
{
    for (auto* id : kAllIds)
        apvts.addParameterListener (id, this);

    applyPreset (0);
}

LeimAudioProcessor::~LeimAudioProcessor()
{
    for (auto* id : kAllIds)
        apvts.removeParameterListener (id, this);
}

void LeimAudioProcessor::prepareToPlay (double sampleRate, int)
{
    lastParams = reader.read();
    dsp.setParams (lastParams);
    dsp.prepare (sampleRate);
    paramsInitialised = true;
}

bool LeimAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LeimAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() == 0)
        return;

    const auto p = reader.read();
    if (! paramsInitialised || ! (p == lastParams))
    {
        dsp.setParams (p);
        lastParams = p;
        paramsInitialised = true;
    }

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    spectrumIn.push (left, right, n);
    dsp.process (left, right, n);
    spectrumOut.push (left, right, n);

    const auto m = dsp.takeMeter();
    storeMax (meterInPeak, m.inPeak);
    storeMax (meterOutPeak, m.outPeak);
    storeMax (meterGr, m.gainReduction);
    if (m.n > 0)
    {
        meterInRms.store  ((float) std::sqrt (m.inSq / m.n));
        meterOutRms.store ((float) std::sqrt (m.outSq / m.n));
        meterDryRms.store ((float) std::sqrt (m.drySq / m.n));
    }
    blockCounter.fetch_add (1);
}

void LeimAudioProcessor::storeMax (std::atomic<float>& a, float v) noexcept
{
    float prev = a.load();
    while (v > prev && ! a.compare_exchange_weak (prev, v)) {}
}

//==============================================================================
int LeimAudioProcessor::getNumPrograms() { return leim::totalPresetCount(); }

void LeimAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < getNumPrograms())
        applyPreset (index);
}

const juce::String LeimAudioProcessor::getProgramName (int index)
{
    int g = 0, p = 0;
    if (! leim::locatePreset (index, g, p))
        return {};
    const auto& genre = leim::presetGenres()[(size_t) g];
    return U (genre.name) + U (" \xc2\xb7 ") + U (genre.presets[(size_t) p].name);
}

void LeimAudioProcessor::applyPreset (int globalIndex)
{
    int g = 0, pi = 0;
    if (! leim::locatePreset (globalIndex, g, pi))
        return;

    const auto& pr = leim::presetGenres()[(size_t) g].presets[(size_t) pi];

    auto set = [this] (const char* id, double value)
    {
        if (auto* param = apvts.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) value));
            param->endChangeGesture();
        }
    };

    applyingPreset = true;
    set (ids::threshold, pr.threshold);
    set (ids::ratio,     choices::nearestIndex (choices::ratio, pr.ratio));
    set (ids::attack,    choices::nearestIndex (choices::attack, pr.attackMs));
    set (ids::release,   choices::nearestIndex (choices::release, pr.releaseS));
    set (ids::makeup,    pr.makeup);
    set (ids::range,     pr.range);
    set (ids::scHpf,     choices::nearestIndex (choices::scHpf, pr.scHpf));
    set (ids::compMix,   pr.compMix);
    set (ids::xover,     pr.xover);
    set (ids::loStyle,   pr.loStyle);
    set (ids::loDrive,   pr.loDrive);
    set (ids::loLevel,   pr.loLevel);
    set (ids::hiStyle,   pr.hiStyle);
    set (ids::hiDrive,   pr.hiDrive);
    set (ids::hiLevel,   pr.hiLevel);
    set (ids::satMix,    pr.satMix);
    set (ids::autoGain,  1.0);
    set (ids::inGain,    0.0);
    set (ids::outGain,   pr.outGain);
    set (ids::softClip,  pr.softClip ? 1.0 : 0.0);
    set (ids::order,     pr.order);
    applyingPreset = false;

    currentProgram = globalIndex;
    presetDirty = false;
}

void LeimAudioProcessor::parameterChanged (const juce::String& id, float)
{
    if (applyingPreset.load() || id == ids::bypass || id == ids::oversampling)
        return;
    presetDirty = true;
}

//==============================================================================
void LeimAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    state.setProperty ("modified", presetDirty.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LeimAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    applyingPreset = true;
    apvts.replaceState (tree);
    applyingPreset = false;

    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    presetDirty = (bool) tree.getProperty ("modified", false);
}

//==============================================================================
juce::AudioProcessorEditor* LeimAudioProcessor::createEditor()
{
    return new LeimEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LeimAudioProcessor();
}
