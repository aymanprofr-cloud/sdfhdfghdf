#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VelvetLookAndFeel.h"

// Bouton rotatif + nom + valeur
class Knob : public juce::Component
{
public:
    Knob (VelvetVerbProcessor& p, const char* paramID, const juce::String& title,
          juce::Colour a, juce::Colour b, bool isBig);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String name;
    bool big;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Visualisation de la queue de reverb
class DecayDisplay : public juce::Component
{
public:
    explicit DecayDisplay (VelvetVerbProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void setLevel (float l) { level = l; }
private:
    float value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    VelvetVerbProcessor& proc;
    float level = 0.0f;
};

class VelvetVerbEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VelvetVerbEditor (VelvetVerbProcessor&);
    ~VelvetVerbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectPreset (int index);

    VelvetVerbProcessor& proc;
    VelvetLookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    DecayDisplay display;

    Knob size, decay, predelay, mix;
    Knob damping, lowCut, highCut, width, duck;

    int shownPreset = -1;
    float smoothedLevel = 0.0f;
    float lastHash = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetVerbEditor)
};
