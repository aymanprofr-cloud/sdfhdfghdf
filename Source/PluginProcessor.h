#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ReverbEngine.h"
#include "Presets.h"

namespace ParamIDs
{
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* predelay = "predelay";
    inline constexpr const char* size     = "size";
    inline constexpr const char* decay    = "decay";
    inline constexpr const char* damping  = "damping";
    inline constexpr const char* lowCut   = "lowcut";
    inline constexpr const char* highCut  = "highcut";
    inline constexpr const char* width    = "width";
    inline constexpr const char* duck     = "duck";
}

class VelvetVerbProcessor : public juce::AudioProcessor
{
public:
    VelvetVerbProcessor();
    ~VelvetVerbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    // presets (visibles aussi dans le menu presets de FL Studio)
    int getNumPrograms() override { return velvet::kNumPresets; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getWetLevel() const noexcept { return engine.getWetLevel(); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentPreset { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float plainValue);

    velvet::ReverbEngine engine;
    juce::AudioBuffer<float> monoScratch;

    std::atomic<float>* pMix = nullptr; std::atomic<float>* pPre = nullptr; std::atomic<float>* pSize = nullptr;
    std::atomic<float>* pDecay = nullptr; std::atomic<float>* pDamp = nullptr; std::atomic<float>* pLow = nullptr;
    std::atomic<float>* pHigh = nullptr; std::atomic<float>* pWidth = nullptr; std::atomic<float>* pDuck = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetVerbProcessor)
};
