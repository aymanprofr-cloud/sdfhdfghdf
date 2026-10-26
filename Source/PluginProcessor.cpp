#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> skewed (float lo, float hi, float centre, float step = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String pct (float v, int)  { return juce::String (juce::roundToInt (v)) + " %"; }
    juce::String ms  (float v, int)  { return juce::String (juce::roundToInt (v)) + " ms"; }
    juce::String sec (float v, int)  { return v < 10.0f ? juce::String (v, 2) + " s" : juce::String (v, 1) + " s"; }
    juce::String hz  (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz"
                            : juce::String (juce::roundToInt (v)) + " Hz";
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VelvetVerbProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using A = juce::AudioParameterFloatAttributes;
    const auto& d = velvet::kPresets[0];

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::mix, 1 },      "Mix",       juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), d.mix,      A().withStringFromValueFunction (pct).withLabel ("%")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::predelay, 1 }, "Pre-Delay", skewed (0.0f, 250.0f, 40.0f, 0.1f),                    d.predelay, A().withStringFromValueFunction (ms).withLabel ("ms")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::size, 1 },     "Size",      juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), d.size,     A().withStringFromValueFunction (pct).withLabel ("%")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::decay, 1 },    "Decay",     skewed (0.2f, 12.0f, 2.0f, 0.01f),                    d.decay,    A().withStringFromValueFunction (sec).withLabel ("s")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::damping, 1 },  "Damping",   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), d.damping,  A().withStringFromValueFunction (pct).withLabel ("%")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::lowCut, 1 },   "Low Cut",   skewed (20.0f, 1000.0f, 200.0f, 1.0f),                d.lowCut,   A().withStringFromValueFunction (hz).withLabel ("Hz")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::highCut, 1 },  "High Cut",  skewed (1000.0f, 20000.0f, 6000.0f, 1.0f),            d.highCut,  A().withStringFromValueFunction (hz).withLabel ("Hz")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::width, 1 },    "Width",     juce::NormalisableRange<float> (0.0f, 150.0f, 0.1f), d.width,    A().withStringFromValueFunction (pct).withLabel ("%")));
    ps.push_back (std::make_unique<P> (juce::ParameterID { ParamIDs::duck, 1 },     "Ducking",   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), d.duck,     A().withStringFromValueFunction (pct).withLabel ("%")));
    return { ps.begin(), ps.end() };
}

VelvetVerbProcessor::VelvetVerbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VelvetVerb", createLayout())
{
    pMix   = apvts.getRawParameterValue (ParamIDs::mix);
    pPre   = apvts.getRawParameterValue (ParamIDs::predelay);
    pSize  = apvts.getRawParameterValue (ParamIDs::size);
    pDecay = apvts.getRawParameterValue (ParamIDs::decay);
    pDamp  = apvts.getRawParameterValue (ParamIDs::damping);
    pLow   = apvts.getRawParameterValue (ParamIDs::lowCut);
    pHigh  = apvts.getRawParameterValue (ParamIDs::highCut);
    pWidth = apvts.getRawParameterValue (ParamIDs::width);
    pDuck  = apvts.getRawParameterValue (ParamIDs::duck);
}

bool VelvetVerbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void VelvetVerbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    monoScratch.setSize (1, samplesPerBlock);
}

void VelvetVerbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    velvet::ReverbParams p;
    p.mix        = pMix->load() * 0.01f;
    p.predelayMs = pPre->load();
    p.size       = pSize->load() * 0.01f;
    p.decaySec   = pDecay->load();
    p.damping    = pDamp->load() * 0.01f;
    p.lowCutHz   = pLow->load();
    p.highCutHz  = pHigh->load();
    p.width      = pWidth->load() * 0.01f;
    p.duck       = pDuck->load() * 0.01f;
    engine.setParams (p);

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() >= 2)
    {
        engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        if (monoScratch.getNumSamples() < n) monoScratch.setSize (1, n, false, false, true);
        auto* l = buffer.getWritePointer (0);
        auto* r = monoScratch.getWritePointer (0);
        juce::FloatVectorOperations::copy (r, l, n);
        engine.process (l, r, n);
        juce::FloatVectorOperations::add (l, r, n);
        juce::FloatVectorOperations::multiply (l, 0.5f, n);
    }
}

void VelvetVerbProcessor::setParam (const char* id, float plainValue)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
}

void VelvetVerbProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= velvet::kNumPresets) return;
    const auto& pr = velvet::kPresets[index];
    currentPreset = index;
    setParam (ParamIDs::mix,      pr.mix);
    setParam (ParamIDs::predelay, pr.predelay);
    setParam (ParamIDs::size,     pr.size);
    setParam (ParamIDs::decay,    pr.decay);
    setParam (ParamIDs::damping,  pr.damping);
    setParam (ParamIDs::lowCut,   pr.lowCut);
    setParam (ParamIDs::highCut,  pr.highCut);
    setParam (ParamIDs::width,    pr.width);
    setParam (ParamIDs::duck,     pr.duck);
}

const juce::String VelvetVerbProcessor::getProgramName (int index)
{
    if (index < 0 || index >= velvet::kNumPresets) return {};
    return velvet::kPresets[index].name;
}

void VelvetVerbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VelvetVerbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, velvet::kNumPresets - 1, (int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* VelvetVerbProcessor::createEditor()
{
    return new VelvetVerbEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VelvetVerbProcessor();
}
