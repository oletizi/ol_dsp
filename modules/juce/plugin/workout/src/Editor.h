//
// Created by Orion Letizi on 12/18/23.
//

#ifndef OL_DSP_EDITOR_H
#define OL_DSP_EDITOR_H

#include <juce_core/juce_core.h>
#include "juce_audio_processors/juce_audio_processors.h"
#include "PluginProcessor.h"

#define FILTER_CUTOFF "Filter Cutoff"

class SliderControl : public juce::Component {
private:
    juce::Slider *slider_;
    juce::Label *label_;
public:
    SliderControl(juce::String name, juce::Slider *slider, juce::Label *label) : slider_(slider), label_(label) {
        //setSize(100, 100);
        slider_->setSliderStyle(juce::Slider::LinearBarVertical);
        slider_->setRange(0.0, 127.0, 1.0);
        slider_->setTextBoxStyle(juce::Slider::NoTextBox, true, 90, 0);
        slider_->setPopupDisplayEnabled(true, false, this);
        slider_->setValue(1.0);
        slider_->setName(name); // could be done by caller, but this forces it to be set in the ctor so you can't forget
        addAndMakeVisible(slider_);
        addAndMakeVisible(label_);
    }

    ~SliderControl() {}

    void resized() override {
        juce::FlexBox fb;
        fb.flexWrap = juce::FlexBox::Wrap::wrap;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.items.add(juce::FlexItem(*slider_).withMinWidth(50.0f).withMinHeight(100.0f));
        fb.items.add(juce::FlexItem(*label_).withMinWidth(50.0f).withMinHeight(100.0f));
        fb.performLayout(getLocalBounds());
    }

};

class ControlCluster : public juce::Component {
private:
    juce::OwnedArray<juce::Component> children;
    juce::Rectangle<int> &min_size_;
    juce::FlexBox::Direction direction_;

public:
    ControlCluster(juce::Rectangle<int> &min_size, juce::FlexBox::Direction direction) : min_size_(min_size),
                                                                                         direction_(direction) {}

    ~ControlCluster() {
        children.clear(false);
    }

    void addChild(juce::Component *child) {
        addAndMakeVisible(child);
        children.add(child);
    }

    void resized() override {
        juce::FlexBox fb;
        fb.flexWrap = juce::FlexBox::Wrap::wrap;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        fb.flexDirection = direction_;

        for (auto *c: children) {
            fb.items.add(juce::FlexItem(*c).withMinHeight(min_size_.getHeight()).withMinWidth(min_size_.getWidth()));
        }

        fb.performLayout(getLocalBounds());
    }

};

class Editor : public juce::AudioProcessorEditor, private juce::Slider::Listener {
private:
    PluginProcessor &processor_;
    juce::Rectangle<int> min_slider_size = juce::Rectangle<int>(30, 100);
    juce::Label cutoff_label_;
    juce::Slider cutoff_slider_;
    SliderControl cutoff_control_ = SliderControl(FILTER_CUTOFF, &cutoff_slider_, &cutoff_label_);
    ControlCluster filter_cluster_ = ControlCluster(min_slider_size, juce::FlexBox::Direction::row);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor)
public:
    explicit Editor(PluginProcessor &p) : AudioProcessorEditor(p),
                                          processor_(p) {
        setSize(500, 500);

        cutoff_slider_.addListener(this);

        filter_cluster_.addChild(&cutoff_control_);

        addAndMakeVisible(filter_cluster_);

        setResizable(true, false);
    }

    void paint(juce::Graphics &g) override {
        // fill the whole window
        g.fillAll(juce::Colours::white);

        // set the current drawing colour to black
        g.setColour(juce::Colours::grey);

        // set the font size and draw text to the screen
        g.setFont(15.0f);

        g.drawFittedText("F Cutoff", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
    }

    void resized() override {
        juce::FlexBox fb;
        fb.flexWrap = juce::FlexBox::Wrap::wrap;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignContent = juce::FlexBox::AlignContent::flexStart;
        fb.flexDirection = juce::FlexBox::Direction::column;

        fb.items.add(juce::FlexItem(filter_cluster_)
                             .withMinHeight(100.0f)
                             .withMinWidth(100.0f));
        fb.performLayout(getLocalBounds());
    }

private:
    void sliderValueChanged(juce::Slider *slider) override {
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
