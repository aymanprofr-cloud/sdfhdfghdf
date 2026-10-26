#include "PluginEditor.h"

// ============================================================================
//  Knob
// ============================================================================
Knob::Knob (VelvetVerbProcessor& p, const char* paramID, const juce::String& title,
            juce::Colour a, juce::Colour b, bool isBig)
    : name (title), big (isBig)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, a);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, b);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityModeParameters (0.6, 1, 0.08, true, juce::ModifierKeys::shiftModifier);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramID, slider);

    if (auto* param = p.apvts.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    slider.onValueChange = [this] { repaint(); };
    slider.setPopupDisplayEnabled (false, false, nullptr);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int textH = big ? 40 : 34;
    auto knobArea = r.removeFromTop (r.getHeight() - textH);
    const int s = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
    slider.setBounds (knobArea.withSizeKeepingCentre (s, s));
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    const int textH = big ? 40 : 34;
    auto text = r.removeFromBottom (textH);

    g.setColour (Theme::textDim);
    g.setFont (Theme::font (big ? 12.5f : 11.0f, true));
    g.drawText (name.toUpperCase(), text.removeFromTop (textH / 2), juce::Justification::centred);

    g.setColour (slider.isMouseOverOrDragging() ? juce::Colours::white : Theme::text);
    g.setFont (Theme::font (big ? 15.0f : 13.0f, true));
    g.drawText (slider.getTextFromValue (slider.getValue()), text, juce::Justification::centredTop);
}

// ============================================================================
//  DecayDisplay
// ============================================================================
void DecayDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (Theme::panelBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

    const float pre   = value (ParamIDs::predelay) * 0.001f;
    const float decay = value (ParamIDs::decay);
    const float size  = value (ParamIDs::size) * 0.01f;
    const float mix   = value (ParamIDs::mix) * 0.01f;
    const float low   = value (ParamIDs::lowCut);
    const float high  = value (ParamIDs::highCut);
    const float damp  = value (ParamIDs::damping) * 0.01f;

    auto plot = r.reduced (18.0f, 14.0f);
    auto band = plot.removeFromBottom (14.0f);
    plot.removeFromBottom (8.0f);
    plot.removeFromTop (18.0f);

    // grille
    g.setColour (juce::Colour (0x0dffffff));
    for (int i = 1; i < 4; ++i)
    {
        const float yy = plot.getY() + plot.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) yy, plot.getX(), plot.getRight());
    }

    const float window = juce::jlimit (1.0f, 14.0f, (pre + decay) * 1.15f);
    auto tx = [&] (float t) { return plot.getX() + plot.getWidth() * (t / window); };
    const float peakH = 0.30f + 0.70f * juce::jmin (1.0f, mix * 2.0f);
    const float attack = 0.01f + 0.07f * size;

    // impulsion seche
    g.setColour (Theme::text.withAlpha (0.55f));
    g.drawLine (plot.getX() + 1.0f, plot.getBottom(), plot.getX() + 1.0f, plot.getY() + plot.getHeight() * 0.05f, 2.0f);

    // enveloppe de la queue
    juce::Path curve, fill;
    const int steps = 220;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = window * (float) i / (float) steps;
        float a = 0.0f;
        if (t > pre)
        {
            const float tt = t - pre;
            const float db = -60.0f * tt / juce::jmax (0.05f, decay);
            const float rise = 1.0f - std::exp (-tt / attack);
            a = rise * juce::jmax (0.0f, 1.0f + db / 60.0f);
            a = std::pow (a, 1.0f + damp * 0.8f);
        }
        const float yy = plot.getBottom() - a * peakH * plot.getHeight();
        if (i == 0) { curve.startNewSubPath (tx (t), yy); fill.startNewSubPath (tx (t), plot.getBottom()); fill.lineTo (tx (t), yy); }
        else        { curve.lineTo (tx (t), yy); fill.lineTo (tx (t), yy); }
    }
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.closeSubPath();

    const float glow = juce::jlimit (0.0f, 1.0f, level * 3.0f);
    juce::ColourGradient fg (Theme::accentA.withAlpha (0.45f + 0.25f * glow), plot.getX(), plot.getY(),
                             Theme::accentB.withAlpha (0.04f), plot.getRight(), plot.getBottom(), false);
    g.setGradientFill (fg);
    g.fillPath (fill);

    juce::ColourGradient lg (Theme::accentA, plot.getX(), 0, Theme::accentB, plot.getRight(), 0, false);
    g.setGradientFill (lg);
    g.setOpacity (0.25f + 0.35f * glow);
    g.strokePath (curve, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setOpacity (1.0f);
    g.setGradientFill (lg);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // marqueur de pre-delay
    if (pre > 0.002f)
    {
        g.setColour (Theme::textDim.withAlpha (0.5f));
        const float px = tx (pre);
        for (float yy = plot.getY(); yy < plot.getBottom(); yy += 6.0f)
            g.drawLine (px, yy, px, yy + 3.0f, 1.0f);
    }

    // infos en haut
    auto info = r.reduced (18.0f, 10.0f).removeFromTop (18.0f);
    g.setFont (Theme::font (11.5f, true));
    g.setColour (Theme::textDim);
    g.drawText ("TAIL", info, juce::Justification::centredLeft);
    g.setColour (Theme::text);
    g.drawText ("RT60  " + juce::String (decay, 2) + " s     PRE  " + juce::String (juce::roundToInt (pre * 1000.0f))
                  + " ms     WINDOW  " + juce::String (window, 1) + " s",
                info, juce::Justification::centredRight);

    // bande de frequences de la reverb (20 Hz - 20 kHz, echelle log)
    auto fx = [&] (float f) { return band.getX() + band.getWidth() * (std::log10 (f / 20.0f) / 3.0f); };
    g.setColour (juce::Colour (0x14ffffff));
    g.fillRoundedRectangle (band.withHeight (4.0f).withY (band.getCentreY() - 2.0f), 2.0f);
    auto active = juce::Rectangle<float> (fx (low), band.getCentreY() - 2.0f, fx (high) - fx (low), 4.0f);
    juce::ColourGradient bg (Theme::accentC, active.getX(), 0, Theme::accentA, active.getRight(), 0, false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (active, 2.0f);
    g.setFont (Theme::font (10.0f, true));
    g.setColour (Theme::textDim);
    g.drawText ("20 Hz", band.withWidth (60.0f).translated (0, -11.0f), juce::Justification::centredLeft);
    g.drawText ("20 kHz", band.withX (band.getRight() - 60.0f).withWidth (60.0f).translated (0, -11.0f), juce::Justification::centredRight);
}

// ============================================================================
//  Editor
// ============================================================================
VelvetVerbEditor::VelvetVerbEditor (VelvetVerbProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p),
      size     (p, ParamIDs::size,     "Size",      Theme::accentA, Theme::accentB, true),
      decay    (p, ParamIDs::decay,    "Decay",     Theme::accentA, Theme::accentB, true),
      predelay (p, ParamIDs::predelay, "Pre-Delay", Theme::accentA, Theme::accentB, true),
      mix      (p, ParamIDs::mix,      "Mix",       Theme::accentB, juce::Colour (0xffffb36b), true),
      damping  (p, ParamIDs::damping,  "Damping",   Theme::accentC, Theme::accentA, false),
      lowCut   (p, ParamIDs::lowCut,   "Low Cut",   Theme::accentC, Theme::accentA, false),
      highCut  (p, ParamIDs::highCut,  "High Cut",  Theme::accentC, Theme::accentA, false),
      width    (p, ParamIDs::width,    "Width",     Theme::accentC, Theme::accentA, false),
      duck     (p, ParamIDs::duck,     "Ducking",   Theme::accentC, Theme::accentA, false)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < velvet::kNumPresets; ++i)
        presetBox.addItem (velvet::kPresets[i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this] { selectPreset ((proc.getCurrentProgram() + velvet::kNumPresets - 1) % velvet::kNumPresets); };
    nextBtn.onClick = [this] { selectPreset ((proc.getCurrentProgram() + 1) % velvet::kNumPresets); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    addAndMakeVisible (display);
    for (auto* k : { &size, &decay, &predelay, &mix, &damping, &lowCut, &highCut, &width, &duck })
        addAndMakeVisible (k);

    setSize (760, 500);
    startTimerHz (30);
}

VelvetVerbEditor::~VelvetVerbEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VelvetVerbEditor::selectPreset (int index)
{
    if (index < 0 || index >= velvet::kNumPresets) return;
    proc.setCurrentProgram (index);
    proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    shownPreset = index;
}

void VelvetVerbEditor::timerCallback()
{
    const int cp = proc.getCurrentProgram();
    if (cp != shownPreset)
    {
        shownPreset = cp;
        presetBox.setSelectedId (cp + 1, juce::dontSendNotification);
    }

    const float lvl = proc.getWetLevel();
    smoothedLevel += (lvl - smoothedLevel) * (lvl > smoothedLevel ? 0.5f : 0.12f);
    display.setLevel (smoothedLevel);

    float hash = smoothedLevel;
    for (auto* id : { ParamIDs::mix, ParamIDs::predelay, ParamIDs::size, ParamIDs::decay,
                      ParamIDs::damping, ParamIDs::lowCut, ParamIDs::highCut })
        hash = hash * 1.37f + proc.apvts.getRawParameterValue (id)->load();
    if (hash != lastHash)
    {
        lastHash = hash;
        display.repaint();
    }
}

void VelvetVerbEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    juce::ColourGradient bg (Theme::bgTop, 0, 0, Theme::bgBottom, 0, r.getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // halos de couleur
    {
        juce::ColourGradient glowA (Theme::accentA.withAlpha (0.20f), 90.0f, 40.0f,
                                    Theme::accentA.withAlpha (0.0f), 90.0f + 360.0f, 40.0f, true);
        g.setGradientFill (glowA);
        g.fillRect (r);
        juce::ColourGradient glowB (Theme::accentB.withAlpha (0.13f), r.getRight() - 60.0f, r.getBottom() - 30.0f,
                                    Theme::accentB.withAlpha (0.0f), r.getRight() - 60.0f - 380.0f, r.getBottom() - 30.0f, true);
        g.setGradientFill (glowB);
        g.fillRect (r);
    }

    // logo
    {
        auto f = Theme::font (28.0f, true);
        f.setExtraKerningFactor (0.18f);
        g.setFont (f);
        juce::ColourGradient tg (juce::Colours::white, 24.0f, 0, Theme::accentA.brighter (0.4f), 200.0f, 0, false);
        g.setGradientFill (tg);
        g.drawText ("VELVET", 24, 16, 200, 32, juce::Justification::centredLeft);

        auto f2 = Theme::font (11.0f, true);
        f2.setExtraKerningFactor (0.35f);
        g.setFont (f2);
        g.setColour (Theme::accentB.withAlpha (0.9f));
        g.drawText ("VOCAL REVERB", 26, 46, 200, 14, juce::Justification::centredLeft);
    }

    // separateur entre les deux rangees de boutons
    g.setColour (juce::Colour (0x10ffffff));
    g.drawHorizontalLine (368, 40.0f, r.getWidth() - 40.0f);

    // signature
    g.setColour (Theme::textDim.withAlpha (0.35f));
    g.setFont (Theme::font (10.0f));
    g.drawText ("double-clic = valeur par defaut   |   shift = reglage fin", getLocalBounds().removeFromBottom (18),
                juce::Justification::centred);
}

void VelvetVerbEditor::resized()
{
    auto r = getLocalBounds().reduced (20, 0);

    // en-tete
    auto header = r.removeFromTop (76).withTrimmedTop (18).withTrimmedBottom (18);
    auto presetArea = header.removeFromRight (360);
    nextBtn.setBounds (presetArea.removeFromRight (38).withSizeKeepingCentre (36, 36));
    presetArea.removeFromRight (8);
    prevBtn.setBounds (presetArea.removeFromLeft (38).withSizeKeepingCentre (36, 36));
    presetArea.removeFromLeft (8);
    presetBox.setBounds (presetArea.withSizeKeepingCentre (presetArea.getWidth(), 38));

    // visualisation
    display.setBounds (r.removeFromTop (130));

    // boutons principaux
    r.removeFromTop (14);
    auto bigRow = r.removeFromTop (140);
    const int bw = bigRow.getWidth() / 4;
    for (auto* k : { &size, &decay, &predelay, &mix })
        k->setBounds (bigRow.removeFromLeft (bw).reduced (14, 0));

    // boutons secondaires
    r.removeFromTop (16);
    auto smallRow = r.removeFromTop (104);
    const int sw = smallRow.getWidth() / 5;
    for (auto* k : { &damping, &lowCut, &highCut, &width, &duck })
        k->setBounds (smallRow.removeFromLeft (sw).reduced (24, 0));
}
