#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    const juce::Colour bgTop       { 0xff17121f };
    const juce::Colour bgBottom    { 0xff0b0910 };
    const juce::Colour panel       { 0x14ffffff };
    const juce::Colour panelBorder { 0x1fffffff };
    const juce::Colour text        { 0xffeee8f6 };
    const juce::Colour textDim     { 0x99eee8f6 };
    const juce::Colour accentA     { 0xff9b6bff };   // violet
    const juce::Colour accentB     { 0xffff6fb5 };   // rose
    const juce::Colour accentC     { 0xff5fd0ff };   // bleu glacier (filtres)

    inline juce::Font font (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
}

class VelvetLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VelvetLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x22ffffff));
        setColour (juce::ComboBox::outlineColourId,    Theme::panelBorder);
        setColour (juce::ComboBox::textColourId,       Theme::text);
        setColour (juce::ComboBox::arrowColourId,      Theme::textDim);
        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff1c1726));
        setColour (juce::PopupMenu::textColourId,                  Theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accentA.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
        setColour (juce::TextButton::buttonColourId,   juce::Colour (0x22ffffff));
        setColour (juce::TextButton::textColourOffId,  Theme::text);
        setColour (juce::TextButton::textColourOnId,   Theme::text);
        setColour (juce::Slider::textBoxTextColourId,  Theme::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff1c1726));
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1c1726));
        setColour (juce::TextEditor::textColourId, Theme::text);
        setColour (juce::TextEditor::highlightColourId, Theme::accentA.withAlpha (0.4f));
        setColour (juce::CaretComponent::caretColourId, Theme::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float size   = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const float radius = size * 0.5f;
        const auto  c      = bounds.getCentre();
        const float thick  = juce::jmax (3.0f, radius * 0.11f);
        const float arcR   = radius - thick * 0.5f;
        const float angle  = startAngle + pos * (endAngle - startAngle);

        const auto colA = s.findColour (juce::Slider::rotarySliderFillColourId);
        const auto colB = s.findColour (juce::Slider::rotarySliderOutlineColourId);
        const bool hover = s.isMouseOverOrDragging();

        // piste
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0x18ffffff));
        g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // arc de valeur (avec halo)
        if (pos > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
            juce::ColourGradient grad (colA, c.x - radius, c.y + radius, colB, c.x + radius, c.y - radius, false);

            g.setGradientFill (grad);
            g.setOpacity (hover ? 0.30f : 0.18f);
            g.strokePath (val, juce::PathStrokeType (thick * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setOpacity (1.0f);
            g.setGradientFill (grad);
            g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // corps du bouton
        const float bodyR = radius - thick * 2.1f;
        auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (body.translated (0.0f, bodyR * 0.08f).expanded (bodyR * 0.04f));

        juce::ColourGradient bodyGrad (juce::Colour (0xff2e2740), c.x, c.y - bodyR,
                                       juce::Colour (0xff15111d), c.x, c.y + bodyR, false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (body);

        g.setColour (juce::Colour (hover ? 0x40ffffff : 0x26ffffff));
        g.drawEllipse (body.reduced (0.5f), 1.0f);

        // reflet
        juce::ColourGradient shine (juce::Colour (0x1cffffff), c.x, c.y - bodyR,
                                    juce::Colour (0x00ffffff), c.x, c.y, false);
        g.setGradientFill (shine);
        g.fillEllipse (body.reduced (bodyR * 0.08f));

        // index
        const float r1 = bodyR * 0.38f, r2 = bodyR * 0.80f;
        const juce::Point<float> p1 (c.x + r1 * std::sin (angle), c.y - r1 * std::cos (angle));
        const juce::Point<float> p2 (c.x + r2 * std::sin (angle), c.y - r2 * std::cos (angle));
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawLine ({ p1, p2 }, juce::jmax (2.0f, bodyR * 0.09f));
        g.setColour (colB.withAlpha (0.9f));
        g.fillEllipse (juce::Rectangle<float> (bodyR * 0.14f, bodyR * 0.14f).withCentre (p2));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, h * 0.5f);
        g.setColour (box.isMouseOver (true) ? Theme::accentA.withAlpha (0.7f) : box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r, h * 0.5f, 1.0f);

        juce::Path arrow;
        const float ax = (float) w - h * 0.62f, ay = h * 0.5f, s = h * 0.13f;
        arrow.startNewSubPath (ax - s, ay - s * 0.5f);
        arrow.lineTo (ax, ay + s * 0.6f);
        arrow.lineTo (ax + s, ay - s * 0.5f);
        g.setColour (Theme::textDim);
        g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (box.getHeight() / 2, 0, box.getWidth() - box.getHeight() - 4, box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centred);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::font (15.0f, true); }
    juce::Font getPopupMenuFont() override              { return Theme::font (15.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
        g.setColour (Theme::panelBorder);
        g.drawRect (0, 0, w, h);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (down ? 0x38ffffff : over ? 0x2cffffff : 0x1affffff));
        g.fillEllipse (r);
        g.setColour (over ? Theme::accentA.withAlpha (0.7f) : Theme::panelBorder);
        g.drawEllipse (r, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return Theme::font ((float) h * 0.5f, true); }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (Theme::font (13.0f, true));
        l->setJustificationType (juce::Justification::centred);
        return l;
    }
};
