#pragma once

#include "../ext/osdialog/osdialog.h"
#include "AudioFile/AudioFile.h"
#include "DiscreteKnob.hpp"
#include "LabelledKnob.hpp"
#include "OpenFileButton.hpp"
#include "Utils.hpp"
#include "ReallySmallBlackKnob.hpp"
#include "rack.hpp"
#include <iomanip>
#include <mutex>
#include <stdio.h>
#include <thread>

using namespace rack;

struct Samplah : rack::Module
{
    enum ParamIds
    {
        ON_OFF_PARAM,
        SPEED_PARAM,
        SPEED_CV_DEPTH,
        NUM_PARAMS
    };

    enum InputIds
    {
        RESTART_TRIGGER,
        SPEED_CV_AMOUNT,
        NUM_INPUTS
    };

    enum OutputIds
    {
        AUDIO_OUTPUT_L,
        AUDIO_OUTPUT_R,
        NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
    };

    Samplah()
        : rack::Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
    {
    }

    json_t* toJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "file", json_string(filename_.c_str()));
        return rootJ;
    }

    void fromJson(json_t* rootJ) override
    {
        json_t* file = json_object_get(rootJ, "file");
        if (file)
        {
            load_sample(json_string_value(file));
        }
    }

    std::vector<std::vector<float>> samples_;
    std::mutex mutex_;
    bool running = false;
    cyclic_iterator<std::vector<float>> left_;
    cyclic_iterator<std::vector<float>> right_;
    std::string filename_;


    void step() override
    {
        if (params[ON_OFF_PARAM].value == 0.0)
        {
            return;
        }

        if (inputs[RESTART_TRIGGER].value >= 1.0)
        {
            left_.idx_ = 0.0;
            right_.idx_ = 0.0;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto cv_mod = 1.0;
        if (inputs[SPEED_CV_AMOUNT].active)
        {          
            cv_mod = rescalef(params[SPEED_CV_DEPTH].value * inputs[SPEED_CV_AMOUNT].value, 0.0, 10.0, -1.0, 1.0);
        }

        if (left_)
        {
            left_ += params[SPEED_PARAM].value * cv_mod;
            outputs[AUDIO_OUTPUT_L].value = *left_;
        }

        if (right_)
        {
            right_ += params[SPEED_PARAM].value * cv_mod;
            outputs[AUDIO_OUTPUT_R].value = *right_;
        }
    }

    void load_sample(const std::string& path)
    {
        AudioFile<float> tmp;
        if (!tmp.load(path))
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        samples_.clear();
        samples_ = std::move(tmp.samples);
        left_.reset();
        right_.reset();
        if (samples_.size() >= 1)
        {
            left_ = cyclic_iterator<std::vector<float>>(samples_[0]);
            if (samples_.size() >= 2)
            {
                right_ = cyclic_iterator<std::vector<float>>(samples_[1]);
            }
        }
        filename_ = path;
    }
};



struct SamplahWidget : rack::ModuleWidget
{

    SamplahWidget()
    {
        extern rack::Plugin* plugin;

        auto module = new Samplah();
        setModule(module);
        box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        auto panel = new SVGPanel();
        panel->setBackground(SVG::load(assetPlugin(plugin,"res/MyModule.svg")));
        addChild(panel);

        addChild(createScrew<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createScrew<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createScrew<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createScrew<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createLabel<CenteredLabel>(Vec(23, 20), "Looper"));
        addChild(createParam<rack::NKK>(Vec(32, 48), module, Samplah::ON_OFF_PARAM, 0.0, 1.0, 1.0));

        auto open_file = new OpenFileButton(
            [module](const std::string& path) 
            { 
                module->load_sample(path);
            });
        open_file->box.pos = Vec(40, 98);
        addChild(open_file);
        addChild(createLabel<CenteredLabel>(Vec(23, 65), "Open File"));
     
    
        using SpeedKnob = LabelledKnob<rack::RoundBlackKnob>;

        auto playback_speed = dynamic_cast<SpeedKnob*>(createParam<SpeedKnob>(Vec(28, 140), module, Samplah::SPEED_PARAM, -2.0, 2.0, 1.0));
        addParam(playback_speed);

        auto l = createLabel<Label>(Vec(3, 180), "Speed");
        addChild(l);
        playback_speed->setLabel(l, [](float v) { return std::string("Speed " + to_string_with_precision(v, 3)); });


        addInput(createInput<PJ301MPort>(Vec(15, 203), module, Samplah::SPEED_CV_AMOUNT));
        addParam(createParam<ReallySmallBlackKnob>(Vec(52, 205), module, Samplah::SPEED_CV_DEPTH, 0.0, 1.0, 1.0));
        addChild(createLabel(Vec(7, 228), "Speed Mod"));

        addInput(createInput<PJ301MPort>(Vec(33, 260), module, Samplah::RESTART_TRIGGER));
        addChild(createLabel(Vec(24, 285), "Sync"));

        addOutput(createOutput<CL1362Port>(Vec(10, 310), module, Samplah::AUDIO_OUTPUT_L));
        addChild(createLabel(Vec(15, 340), "L"));

        addOutput(createOutput<CL1362Port>(Vec(50, 310), module, Samplah::AUDIO_OUTPUT_R));
        addChild(createLabel(Vec(55, 340), "R"));
    }
};