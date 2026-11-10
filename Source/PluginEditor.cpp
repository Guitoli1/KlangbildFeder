#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
FederAudioProcessorEditor::FederAudioProcessorEditor (FederAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    dwellKnob = &addKnob ("dwell", "DWELL", true);
    mixKnob   = &addKnob ("mix", "MISCHUNG", true);
    rowKnobs = { &addKnob ("tank", "TANK", false), &addKnob ("decay", u8 ("L\xc3\x84NGE"), false), &addKnob ("tone", "TON", false),
                 &addKnob ("chirp", "ZWITSCHERN", false), &addKnob ("predelay", u8 ("VORVERZ\xc3\x96GERUNG"), false), &addKnob ("lowcut", "BASSFILTER", false) };
    rowKnobs[0]->slider.getProperties().set ("stepLabels", u8 ("KURZ|LANG|GRO\xc3\x9f"));
    rowKnobs[0]->slider.setMouseDragSensitivity (100);
    rowKnobs[1]->slider.getProperties().set ("stepLabels", "0,5|1,9|3,3|4,6|6 S");
    rowKnobs[4]->slider.getProperties().set ("stepLabels", "0|20|40|60|80");
    rowKnobs[5]->slider.getProperties().set ("stepLabels", "60|90|200|360|600");

    setupSwitch (sendSwitch, "send", "NUR HALL", "MIT DIREKT", lampAmber);
    kickButton.setButtonText (u8 ("STOSS"));
    kickButton.setTooltip (u8 ("Wie ein Tritt gegen den Verst\xc3\xa4rker"));
    kickButton.onClick = [this] { proc.kick(); };
    addAndMakeVisible (kickButton);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    springWin.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (springWin);

    setSize (1100, 650);
    startTimerHz (40);
}

FederAudioProcessorEditor::~FederAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

FederAudioProcessorEditor::Knob& FederAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void FederAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lampColour)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lampColour.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

//==============================================================================
void FederAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void FederAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void FederAudioProcessorEditor::timerCallback()
{
    springWin.time += 1.0 / 40.0;
    const int k = proc.kickCounter.load();
    if (k != lastKick) { lastKick = k; springWin.shake = 1.0f; }
    springWin.shake *= 0.9f;
    springWin.level = juce::jlimit (0.0f, 1.0f, proc.wetLevel.load() * 6.0f);
    const int sp = (int) proc.apvts.getRawParameterValue ("tank")->load() == 0 ? 2 : 3;
    springWin.springs = sp;
    springWin.repaint();

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void FederAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("FEDER", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 160.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("FEDERHALL \xc2\xb7 R\xc3\x96HRENTREIBER \xc2\xb7 DREI TANKS"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);
    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    const auto wb = winBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (wb.translated (2.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, wb.getY(), gold.darker (0.55f), 0, wb.getBottom(), false));
    g.fillRoundedRectangle (wb, 12.0f);
    for (auto pt : { wb.getTopLeft().translated (11, 11), wb.getTopRight().translated (-11, 11),
                     wb.getBottomLeft().translated (11, -11), wb.getBottomRight().translated (-11, -11) })
        VintageLookAndFeel::drawScrew (g, pt, 4.0f, pt.x * 0.013f);

    const auto dv = divider.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine ((int) dv.getY() + 1, dv.getX(), dv.getRight());
    g.setColour (ink.withAlpha (0.45f));
    g.drawHorizontalLine ((int) dv.getY(), dv.getX(), dv.getRight());

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 12.0f, k->big ? 0.2f : 0.12f), juce::Justification::centred);
    }
    drawEngraved (g, "AUSGANG", sendLabel.toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    drawEngraved (g, u8 ("VERST\xc3\x84RKER ANSTOSSEN"), kickLabel.toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);

    const auto tag = tagArea.toFloat();
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-6G15", tag, juce::Justification::centred);
}

void FederAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);
    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (34, 22);

    dwellKnob->slider.setBounds (in.getX() + 20, in.getY() + 6, 220, 200);
    mixKnob->slider.setBounds (in.getRight() - 240, in.getY() + 6, 220, 200);
    winBezel = juce::Rectangle<int> (380, 200).withCentre ({ in.getCentreX(), in.getY() + 104 });
    springWin.setBounds (winBezel.reduced (16));

    divider = { in.getX() + 10, in.getY() + 234, in.getWidth() - 20, 2 };

    const int ry = in.getY() + 256;
    constexpr int kw = 108, kh = 108, rowW = 700;
    for (int i = 0; i < 6; ++i)
        rowKnobs[(size_t) i]->slider.setBounds (in.getX() + i * (rowW / 6) + (rowW / 6 - kw) / 2, ry, kw, kh);

    const int sx = in.getX() + rowW + 24;
    sendLabel = { sx, ry, 220, 16 };
    sendSwitch.setBounds (sx, ry + 18, 190, 40);
    kickLabel = { sx, ry + 70, 240, 16 };
    kickButton.setBounds (sx, ry + 88, 120, 30);
    tagArea = juce::Rectangle<int> (130, 24).withPosition (in.getRight() - 130, ry + 92);
    renderTextures();
}

//==============================================================================
void FederAudioProcessorEditor::SpringWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23201c), 0, b.getY(), juce::Colour (0xff0c0b09), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);

    // Bodenblech mit Lochreihe
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    for (float x = b.getX() + 14.0f; x < b.getRight() - 10.0f; x += 16.0f)
        g.fillEllipse (x, b.getBottom() - 12.0f, 5.0f, 5.0f);

    // Wandler links (IN) und rechts (OUT)
    const float x0 = b.getX() + 38.0f, x1 = b.getRight() - 38.0f;
    for (float side : { 0.0f, 1.0f })
    {
        const auto t = juce::Rectangle<float> (30.0f, b.getHeight() - 30.0f).withCentre ({ side < 0.5f ? x0 - 16.0f : x1 + 16.0f, b.getCentreY() - 4.0f });
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a8f95), t.getX(), 0, juce::Colour (0xff3b3e42), t.getRight(), 0, false));
        g.fillRoundedRectangle (t, 3.0f);
        g.setColour (juce::Colour (0xffd9c48f).withAlpha (0.8f));
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (side < 0.5f ? "IN" : "OUT", t.withHeight (14.0f).withY (t.getBottom() - 16.0f), juce::Justification::centred);
    }

    // Federn
    const int n = juce::jlimit (2, 3, springs);
    const float span = b.getHeight() - 44.0f;
    for (int s = 0; s < n; ++s)
    {
        const float cy = b.getY() + 16.0f + span * ((float) s + 0.5f) / (float) n;
        const float amp = (1.0f + 9.0f * level + 16.0f * shake) * (0.8f + 0.2f * (float) s);
        const double f1 = 2.3 + 0.7 * s, f2 = 5.9 + 1.3 * s;
        const float coilR = 5.0f;
        juce::Path back, front;
        const int steps = 360;
        for (int i = 0; i <= steps; ++i)
        {
            const float u = (float) i / (float) steps;
            const float x = x0 + u * (x1 - x0);
            const float bow = std::sin (juce::MathConstants<float>::pi * u);
            const float wob = amp * bow * (float) (0.7 * std::sin (6.2831853 * f1 * time + s) + 0.3 * std::sin (6.2831853 * f2 * time + 2.0 * s));
            const float ang = u * 46.0f * juce::MathConstants<float>::twoPi;
            const float y = cy + wob + coilR * std::sin (ang);
            auto& p = std::cos (ang) > 0.0f ? front : back;
            if (i == 0 || (std::cos (ang) > 0.0f) != (std::cos ((float) (i - 1) / (float) steps * 46.0f * juce::MathConstants<float>::twoPi) > 0.0f))
                p.startNewSubPath (x, y);
            else
                p.lineTo (x, y);
        }
        g.setColour (juce::Colour (0xff5d6066));
        g.strokePath (back, juce::PathStrokeType (1.3f));
        g.setColour (juce::Colour (0xffd0d4d9).withAlpha (0.92f));
        g.strokePath (front, juce::PathStrokeType (1.6f));
        g.setColour (juce::Colours::white.withAlpha (0.25f + 0.4f * level));
        g.strokePath (front, juce::PathStrokeType (0.6f), juce::AffineTransform::translation (0.0f, -1.2f));
    }

    // Glanz auf der Scheibe
    juce::Path refl;
    refl.addTriangle (b.getX(), b.getY(), b.getX() + b.getWidth() * 0.45f, b.getY(), b.getX(), b.getY() + b.getHeight() * 0.7f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillPath (refl);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 2.0f);
}
