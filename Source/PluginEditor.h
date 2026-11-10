#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class FederAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FederAudioProcessorEditor (FederAudioProcessor&);
    ~FederAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Sichtfenster in den Hallkasten: die Federn schwingen mit dem Hall
    struct SpringWindow : public juce::Component
    {
        int springs = 3;
        float level = 0.0f, shake = 0.0f;
        double time = 0.0;
        void paint (juce::Graphics&) override;
    };

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lampColour);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    FederAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* dwellKnob = nullptr; Knob* mixKnob = nullptr;
    std::array<Knob*, 6> rowKnobs {};
    juce::ToggleButton sendSwitch;
    juce::TextButton kickButton;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    SpringWindow springWin;
    int lastKick = 0;

    juce::Rectangle<int> faceplate, presetArea, winBezel, divider, sendLabel, kickLabel, tagArea;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FederAudioProcessorEditor)
};
