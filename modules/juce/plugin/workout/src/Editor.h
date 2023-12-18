//
// Created by Orion Letizi on 12/18/23.
//

#ifndef OL_DSP_EDITOR_H
#define OL_DSP_EDITOR_H

#include <juce_core/juce_core.h>
#include "juce_audio_processors/juce_audio_processors.h"
#include "PluginProcessor.h"

#define FILTER_CUTOFF "Filter Cutoff"

class Editor : public juce::AudioProcessorEditor,  private juce::Slider::Listener {
private:
    PluginProcessor &processor_;
    juce::Slider cutoff_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor)
public:
    Editor( PluginProcessor &p) : AudioProcessorEditor(p), processor_(p) {
        setSize(500,500);
        // This is where our plugin’s editor size is set.
        setSize (200, 200);

        // these define the parameters of our slider object
        cutoff_.setSliderStyle (juce::Slider::LinearBarVertical);
        cutoff_.setRange (0.0, 127.0, 1.0);
        cutoff_.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
        cutoff_.setPopupDisplayEnabled (true, false, this);
        cutoff_.setTextValueSuffix (" Cutoff");
        cutoff_.setValue(1.0);
        cutoff_.setName(FILTER_CUTOFF);

        cutoff_.addListener(this);

        // this function adds the slider to the editor
        addAndMakeVisible (&cutoff_);
    }

    void paint(juce::Graphics &g) {
        // fill the whole window white
        g.fillAll (juce::Colours::white);

        // set the current drawing colour to black
        g.setColour (juce::Colours::black);

        // set the font size and draw text to the screen
        g.setFont (15.0f);

        g.drawFittedText ("F Cutoff", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
    }

    void resized() override {
        // sets the position and size of the slider with arguments (x, y, width, height)
        cutoff_.setBounds (40, 30, 20, getHeight() - 60);
    }

private:
    void sliderValueChanged (juce::Slider* slider) override {
        auto name = slider->getName();
        auto value = slider->getValue();
        printf("Name: %s, val: %f\n", name.toRawUTF8(), value);
        if (name == FILTER_CUTOFF) {
            printf("  Cutoff: %f\n", value);
            //processor_.SetFilterCutoff()
            processor_.UpdateHardwareControl(CC_FILTER_CUTOFF, t_sample(value));
        }
    }

};

#endif //OL_DSP_EDITOR_H
