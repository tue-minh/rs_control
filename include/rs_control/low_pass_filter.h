#ifndef LOW_PASS_FILTER_H
#define LOW_PASS_FILTER_H

#pragma once

#include <cmath>

namespace rs_control
{

class LowPassFilter
{
public:
    LowPassFilter()
        : cutoff_freq_(10.0),
          sample_time_(0.02),
          alpha_(0.0),
          initialized_(false),
          previous_output_(0.0)
    {
        updateCoefficient();
    }

    LowPassFilter(double cutoff_freq, double sample_time)
        : cutoff_freq_(cutoff_freq),
          sample_time_(sample_time),
          alpha_(0.0),
          initialized_(false),
          previous_output_(0.0)
    {
        updateCoefficient();
    }

    void setCutoffFrequency(double cutoff_freq)
    {
        cutoff_freq_ = cutoff_freq;
        updateCoefficient();
    }

    void setSampleTime(double dt)
    {
        sample_time_ = dt;
        updateCoefficient();
    }

    void reset(double value = 0.0)
    {
        previous_output_ = value;
        initialized_ = false;
    }

    double filter(double input)
    {
        if (!initialized_)
        {
            previous_output_ = input;
            initialized_ = true;
            return input;
        }

        previous_output_ =
            alpha_ * input +
            (1.0 - alpha_) * previous_output_;

        return previous_output_;
    }

    double output() const
    {
        return previous_output_;
    }

private:

    void updateCoefficient()
    {
        double rc = 1.0 / (2.0 * M_PI * cutoff_freq_);
        alpha_ = sample_time_ / (sample_time_ + rc);
    }

    double cutoff_freq_;
    double sample_time_;

    double alpha_;

    bool initialized_;

    double previous_output_;
};

}

#endif // LOW_PASS_FILTER_H