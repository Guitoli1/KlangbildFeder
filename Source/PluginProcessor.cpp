#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr scale10() { return withText ([] (float v) { return juce::String (v, 1); }); }
}

juce::AudioProcessorValueTreeState::ParameterLayout FederAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "tank", 1 }, "Tank",
        juce::StringArray { u8 ("Kurz \xc2\xb7 2 Federn"), u8 ("Lang \xc2\xb7 3 Federn"), u8 ("Gro\xc3\x9f \xc2\xb7 Studio") }, 1));
    l.add (std::make_unique<APF> (juce::ParameterID { "dwell", 1 }, "Dwell", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, scale10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "decay", 1 }, u8 ("L\xc3\xa4nge"), juce::NormalisableRange<float> (0.5f, 6.0f, 0.01f, 0.5f), 2.2f,
        withText ([] (float v) { return juce::String (v, 1) + " s"; })));
    l.add (std::make_unique<APF> (juce::ParameterID { "tone", 1 }, "Ton", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f, scale10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "chirp", 1 }, "Zwitschern", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f, scale10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "predelay", 1 }, u8 ("Vorverz\xc3\xb6gerung"), juce::NormalisableRange<float> (0.0f, 80.0f, 0.1f), 0.0f,
        withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; })));
    l.add (std::make_unique<APF> (juce::ParameterID { "lowcut", 1 }, "Bassfilter", juce::NormalisableRange<float> (60.0f, 600.0f, 1.0f, 0.5f), 150.0f,
        withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; })));
    l.add (std::make_unique<APF> (juce::ParameterID { "mix", 1 }, "Mischung", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f,
        withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " %"; })));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "send", 1 }, "Nur Hall (Send)", false));
    return l;
}

const std::vector<FederAudioProcessor::Preset>& FederAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                          { { "tank", 1 }, { "dwell", 3 },   { "decay", 2.2f }, { "tone", 5 },   { "chirp", 5 },   { "predelay", 0 },  { "lowcut", 150 }, { "mix", 25 }, { "send", 0 } } },
        { "Blues-Combo",                       { { "tank", 0 }, { "dwell", 4 },   { "decay", 1.6f }, { "tone", 4 },   { "chirp", 5 },   { "predelay", 0 },  { "lowcut", 160 }, { "mix", 20 }, { "send", 0 } } },
        { "Soul-Gitarre",                      { { "tank", 0 }, { "dwell", 3 },   { "decay", 1.8f }, { "tone", 5.5f },{ "chirp", 4 },   { "predelay", 10 }, { "lowcut", 200 }, { "mix", 18 }, { "send", 0 } } },
        { "Surf \xe2\x80\x93 nass & tropfend",  { { "tank", 1 }, { "dwell", 8 },   { "decay", 3.0f }, { "tone", 7 },   { "chirp", 8.5f },{ "predelay", 0 },  { "lowcut", 150 }, { "mix", 45 }, { "send", 0 } } },
        { "Gesang \xe2\x80\x93 Vintage",        { { "tank", 2 }, { "dwell", 2 },   { "decay", 2.5f }, { "tone", 6 },   { "chirp", 3 },   { "predelay", 30 }, { "lowcut", 260 }, { "mix", 20 }, { "send", 0 } } },
        { "Orgel \xe2\x80\x93 Raum",            { { "tank", 2 }, { "dwell", 3 },   { "decay", 3.2f }, { "tone", 4.5f },{ "chirp", 3.5f },{ "predelay", 15 }, { "lowcut", 180 }, { "mix", 30 }, { "send", 0 } } },
        { "Dub \xe2\x80\x93 tiefe Feder",       { { "tank", 1 }, { "dwell", 7 },   { "decay", 4.5f }, { "tone", 3 },   { "chirp", 9 },   { "predelay", 0 },  { "lowcut", 90 },  { "mix", 40 }, { "send", 0 } } },
        { "Snare \xe2\x80\x93 Feder",           { { "tank", 0 }, { "dwell", 5 },   { "decay", 1.2f }, { "tone", 6.5f },{ "chirp", 6 },   { "predelay", 5 },  { "lowcut", 300 }, { "mix", 22 }, { "send", 0 } } },
        { "Send \xe2\x80\x93 100 % Hall",       { { "tank", 1 }, { "dwell", 3 },   { "decay", 2.4f }, { "tone", 5 },   { "chirp", 5 },   { "predelay", 10 }, { "lowcut", 180 }, { "mix", 55 }, { "send", 1 } } },
    };
    return p;
}

void FederAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String FederAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

FederAudioProcessor::FederAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildFeder", createLayout())
{
}

bool FederAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void FederAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    using S = feder::SpringUnit::Spring;
    units[0].prepare (sr, { S { 0.0287, 0.58f, 0.37 }, S { 0.0331, 0.63f, 0.31 } }, 1.25f, 1.32);
    units[1].prepare (sr, { S { 0.0365, 0.60f, 0.41 }, S { 0.0412, 0.64f, 0.29 }, S { 0.0297, 0.57f, 0.53 } }, 1.0f, 1.14);
    units[2].prepare (sr, { S { 0.0470, 0.66f, 0.23 }, S { 0.0531, 0.69f, 0.19 }, S { 0.0412, 0.63f, 0.33 } }, 0.95f, 1.04);
    activeUnit = (int) apvts.getRawParameterValue ("tank")->load();
    fadingUnit = -1;
    fadeLen = (int) (0.04 * sr);
    preDelay.prepare ((int) (0.09 * sr) + 8);
    inHP.setCutoff (120.0, sr);
    inLP.setCutoff (5000.0, sr);
    for (auto& s : lowCutState) s.reset();
    lastLowCut = -1.0f;
    lastUnitParams.fill (-1.0f);
    for (auto* s : { &smDwell, &smMix, &smPre, &smDry })
        s->reset (sr, 0.05);
    smDwell.setCurrentAndTargetValue (apvts.getRawParameterValue ("dwell")->load() * 0.1f);
    smMix.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);
    smPre.setCurrentAndTargetValue (apvts.getRawParameterValue ("predelay")->load() * 0.001f * (float) sr);
    smDry.setCurrentAndTargetValue (apvts.getRawParameterValue ("send")->load() > 0.5f ? 0.0f : 1.0f);
    envCoef = (float) (1.0 - std::exp (-1.0 / (0.03 * sr)));
    env = 0;
    kickPos = -1;
}

void FederAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = juce::jmin (2, getTotalNumOutputChannels());
    if (n == 0 || outCh == 0)
        return;
    if (inCh == 1 && outCh == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    // Tank-Wechsel: kurz überblenden, damit es nicht knackt
    const int tank = juce::jlimit (0, 2, (int) apvts.getRawParameterValue ("tank")->load());
    if (tank != activeUnit && fadingUnit < 0)
    {
        units[(size_t) tank].reset();
        fadingUnit = activeUnit;
        activeUnit = tank;
        fadePos = 0;
    }

    const std::array<float, 4> up { apvts.getRawParameterValue ("decay")->load(), apvts.getRawParameterValue ("tone")->load(),
                                    apvts.getRawParameterValue ("chirp")->load(), (float) tank };
    if (up != lastUnitParams)
    {
        for (auto& u : units)
            u.setParams (up[0], up[1] * 0.1f, up[2] * 0.1f);
        lastUnitParams = up;
    }
    const float lc = apvts.getRawParameterValue ("lowcut")->load();
    if (! juce::exactlyEqual (lc, lastLowCut))
    {
        lowCut = eq::makeHighPass (sr, lc, 0.70710678);
        lastLowCut = lc;
    }

    smDwell.setTargetValue (apvts.getRawParameterValue ("dwell")->load() * 0.1f);
    smMix.setTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);
    smPre.setTargetValue (juce::jmax (4.0f, apvts.getRawParameterValue ("predelay")->load() * 0.001f * (float) sr));
    smDry.setTargetValue (apvts.getRawParameterValue ("send")->load() > 0.5f ? 0.0f : 1.0f);

    if (kickRequested.exchange (false))
    {
        kickPos = 0;
        kickCounter.fetch_add (1);
    }

    float* L = buffer.getWritePointer (0);
    float* R = outCh > 1 ? buffer.getWritePointer (1) : nullptr;
    constexpr float wetGain = 1.3f;
    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R ? R[i] : dl;
        float x = inLP.process (inHP.process (0.5f * (dl + dr)));
        preDelay.push (x);
        x = preDelay.read (smPre.getNextValue());

        // Dwell: wie stark der Röhren-Treiber die Feder anfährt
        const float drive = 1.0f + 5.0f * smDwell.getNextValue();
        x = std::tanh (x * drive) / std::pow (drive, 0.8f);

        // Stoß: die Federn schlagen aneinander – tiefer Schlag plus Rasseln, direkt in den Tank
        if (kickPos >= 0)
        {
            const float t = (float) kickPos / (float) sr;
            x += 2.6f * std::exp (-t * 30.0f) * (std::sin (6.2831853f * 70.0f * t) + 0.8f * (kickRnd.nextFloat() * 2.0f - 1.0f));
            if (++kickPos > (int) (0.15 * sr)) kickPos = -1;
        }

        float wl, wr;
        units[(size_t) activeUnit].process (x, wl, wr);
        if (fadingUnit >= 0)
        {
            float ol, orr;
            units[(size_t) fadingUnit].process (x, ol, orr);
            const float f = (float) fadePos / (float) fadeLen;
            wl = wl * f + ol * (1.0f - f);
            wr = wr * f + orr * (1.0f - f);
            if (++fadePos >= fadeLen) fadingUnit = -1;
        }
        wl = lowCutState[0].process (lowCut, wl);
        wr = lowCutState[1].process (lowCut, wr);

        const float wet = smMix.getNextValue() * wetGain, dry = smDry.getNextValue();
        env += (std::abs (wl) + std::abs (wr) - env) * envCoef;
        if (R)
        {
            L[i] = dry * dl + wet * wl;
            R[i] = dry * dr + wet * wr;
        }
        else
            L[i] = dry * dl + wet * 0.5f * (wl + wr);
    }
    wetLevel.store (env);
}

void FederAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FederAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* FederAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new FederAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FederAudioProcessor(); }
