#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FederDsp.h"

class FederAudioProcessor : public juce::AudioProcessor
{
public:
    FederAudioProcessor();
    ~FederAudioProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 8.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // „Stoß“: als hätte jemand gegen den Verstärker getreten
    void kick() { kickRequested.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> wetLevel { 0 };       // Hüllkurve des Halls für die Feder-Anzeige
    std::atomic<int> kickCounter { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<feder::SpringUnit, 3> units;
    int activeUnit = 1, fadingUnit = -1, fadePos = 0, fadeLen = 1;
    feder::DelayLine preDelay;
    feder::OnePoleHP inHP;
    feder::OnePoleLP inLP;
    std::array<eq::BiquadState, 2> lowCutState;
    eq::Coeffs lowCut;
    float lastLowCut = -1.0f;
    std::array<float, 4> lastUnitParams { -1, -1, -1, -1 };
    std::atomic<bool> kickRequested { false };
    int kickPos = -1;
    juce::Random kickRnd;
    juce::SmoothedValue<float> smDwell, smMix, smPre, smDry;
    float env = 0, envCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FederAudioProcessor)
};
