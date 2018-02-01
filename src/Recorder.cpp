#include "rack.hpp"
#include "Recorder.hpp"
#include "WavWriter.hpp"
#include "OpenFileButton.hpp"
#include "Utils.hpp"
#include <iostream>
#include "dsp/vumeter.hpp"
#include <stdio.h>
#include <chrono>

namespace lufu
{

    template <typename Light, size_t Radius=8>
    struct VUSegment : public Light
    {
        VUSegment() : Light()
        {
            this->box.size = rack::Vec(Radius, Radius);
        }
    };


    class RecorderModule : public rack::Module
    {
    public:
        enum ParamIds
        {
            RECORD_STOP_BUTTON,
            NUM_PARAMS
        };

        enum InputIds
        {
            INPUT_L,
            INPUT_R,
            NUM_INPUTS
        };

        enum OutputIds
        {
            NUM_OUTPUTS
        };

        static constexpr int VU_METER_LIGHTS = 16;

        enum LightIds
        {
            VU_METER_LEFT_1,
            VU_METER_LEFT_END = VU_METER_LEFT_1 + VU_METER_LIGHTS,
            VU_METER_RIGHT_1,
            VU_METER_RIGHT_END = VU_METER_RIGHT_1 + VU_METER_LIGHTS,
            NUM_LIGHTS
        };

    public:
        RecorderModule()
            : rack::Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
        {
            meter_left_.dBInterval = 3;
            meter_right_.dBInterval = 3;
        }

        void set_recording_time_label(rack::Label * label)
        {
            recording_time_label_ = label;
        }

        void on_set_target_file(std::string path)
        {
            target_file_ = std::move(path);
            std::cout << "Saving to filename " << target_file_ << "\n";
        }

        void vu_lights(float left, float right)
        {
            meter_left_.setValue(left / 5.0);
            meter_right_.setValue(right / 5.0);
            for (int l = 0; l < VU_METER_LIGHTS; l++)
            {
                lights[l + VU_METER_LEFT_1].setBrightnessSmooth(meter_left_.getBrightness(VU_METER_LIGHTS - l));
                lights[l + VU_METER_RIGHT_1].setBrightnessSmooth(meter_right_.getBrightness(VU_METER_LIGHTS - l));
            }
        }

        void step() override
        {
            float left = inputs[INPUT_L].active ? inputs[INPUT_L].value : 0.0;
            float right = inputs[INPUT_R].active ? inputs[INPUT_R].value : 0.0;

            vu_lights(left, right);

            if (target_file_.empty() || !params[RECORD_STOP_BUTTON].value)
            {
                if (sink_)
                {
                    sink_.reset();
                }
                return;
            }

            if (!sink_)
            {
                sink_ = std::unique_ptr<WavSink>(new WavSink(target_file_, 44100));
                seconds = 0;
            }

            sink_->push_samples(left, right);

            if (ticks_ % 44100 == 0)
            {
                char time_str_[9];
                const int sec = seconds % 60;
                const int min =  (seconds / 60) % 60;
                const int hour = (seconds / (60 * 60)) % 24;
                sprintf(time_str_, "%02d:%02d:%02d", hour, min, sec);
                recording_time_.assign(time_str_);
                recording_time_label_->text = recording_time_;
                seconds++;
            }
            ticks_++;
        }

    private:
        std::string recording_time_{"00:00:00"};
        rack::Label * recording_time_label_{nullptr};
        uint64_t ticks_{0};
        uint32_t seconds;
        std::string target_file_;
        std::unique_ptr<lufu::WavSink> sink_;
        rack::VUMeter meter_left_;
        rack::VUMeter meter_right_;
    };


    // Adjust position so that is centered horizontally in outer
    void center_horiz(const rack::Widget& other, rack::Widget & that)
    {
        auto center = other.box.size.x / 2;
        auto width = that.box.size.x / 2;

        that.box.pos.x = center - width;
    }

    RecorderWidget::RecorderWidget()
        : BaseModuleWidget("res/Recorder.svg")
    {
        auto rec = createParam<rack::NKK>(Vec(32, 48), module_, RecorderModule::RECORD_STOP_BUTTON, 0.0, 1.0, 1.0);
        center_horiz(*this, *rec);
        addParam(rec);

        auto open_file = new OpenFileButton([this](const std::string& path)
        {
            this->module_->on_set_target_file(path);
        }, OSDIALOG_SAVE);

        open_file->box.pos = Vec(70, 96);
        center_horiz(*this, *open_file);
        addChild(open_file);

        auto time_label = createLabel(Vec(10, 115), "00:00:00");
        addChild(time_label);
        module_->set_recording_time_label(time_label);

        addInput(createInput<PJ301MPort>(Vec(10, 310), module_, RecorderModule::INPUT_L));
        addInput(createInput<PJ301MPort>(Vec(50, 310), module_, RecorderModule::INPUT_R));

        using Green = VUSegment<rack::GreenLight>;
        using Yellow = VUSegment<rack::YellowLight>;
        using Red = VUSegment<rack::RedLight>;

        for (int i = 0; i < RecorderModule::VU_METER_LIGHTS; i++)
        {

            constexpr int leftX = 18;
            constexpr int rightX = 59;

            if (i < 10)
            {
                addChild(createLight<Green>(Vec(leftX, 290 - (i * 10)), module_, RecorderModule::VU_METER_LEFT_1 + i));
                addChild(createLight<Green>(Vec(rightX, 290 - (i * 10)), module_, RecorderModule::VU_METER_RIGHT_1 + i));
            }

            if (i >= 10 && i < 14)
            {
                addChild(createLight<Yellow>(Vec(leftX, 290 - (i * 10)), module_, RecorderModule::VU_METER_LEFT_1 + i));
                addChild(createLight<Yellow>(Vec(rightX, 290 - (i * 10)), module_, RecorderModule::VU_METER_RIGHT_1 + i));
            }

            if (i >= 14)
            {
                addChild(createLight<Red>(Vec(leftX, 290 - (i * 10)), module_, RecorderModule::VU_METER_LEFT_1 + i));
                addChild(createLight<Red>(Vec(rightX, 290 - (i * 10)), module_, RecorderModule::VU_METER_RIGHT_1 + i));
            }

        }
    }
}
