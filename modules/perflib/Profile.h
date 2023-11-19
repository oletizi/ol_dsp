//
// Created by Orion Letizi on 11/18/23.
//

#ifndef OL_DSP_PROFILE_H
#define OL_DSP_PROFILE_H

#include <cstdint>
#include <vector>

struct PerfSample {
    uint64_t start = 0;
    uint64_t end = 0;
    t_sample in1;
    t_sample in2;
    t_sample out1;
    t_sample out2;
    t_sample val1;
};

namespace ol::perflib {
    typedef uint64_t (*TimestampCallback)();

    class Profile {
    public:
        Profile(uint64_t sample_size, TimestampCallback timestamp_callback) :
                sample_size_(sample_size), timestamp_callback_(timestamp_callback) {
            for (int i = 0; i < sample_size; i++) {
                samples_.emplace_back();
            }
        }

        virtual void Start() {
            if (sample_size_ > 0) {
                if (counter_ > sample_size_) {
                    counter_ = 0;
                }
                samples_[counter_].start = timestamp_callback_();
            }
        };

        virtual void End() {
            if (sample_size_ > 0) {
                samples_[counter_].end = timestamp_callback_();
                counter_++;
            }
        };

        void In1(t_sample in1) {
            if (sample_size_ > 0) {
                samples_[counter_].in1 = in1;
            }
        };

        void In2(t_sample in2) {
            if (sample_size_ > 0) {
                samples_[counter_].in2 = in2;
            }
        }

        void Out1(t_sample out1) {
            if (sample_size_ > 0) {
                samples_[counter_].out1 = out1;
            }
        }

        void Out2(t_sample out2) {
            if (sample_size_ > 0) {
                samples_[counter_].out2 = out2;
            }
        }

        void ValA(float val) {
            if (sample_size_ > 0) {
                samples_[counter_].val1 = val;
            }
        }

        double AverageExecutionTime() {
            uint64_t total = 0;
            for (int i = 0; i < sample_size_; i++) {
                total += samples_[i].end - samples_[i].start;
            }
            return sample_size_ == 0 ? 0 : double(total) / double(sample_size_);
        }

        double MaxExecutionTime() {
            uint64_t max = 0;
            for (int i = 0; i < sample_size_; i++) {
                uint64_t elapsed = samples_[i].end - samples_[i].start;
                if (elapsed > max) {
                    max = elapsed;
                }
            }
            return double(max);
        }

        double MaxIn1Value() {
            double max = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].in1;
                if (value > max) {
                    max = value;
                }
            }
            return max;
        }

        double MinIn1Value() {
            double min = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].in1;
                if (value < min) {
                    min = value;
                }
            }
            return min;
        }

        double MaxOut1Value() {
            double max = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].out1;
                if (value > max) {
                    max = value;
                }
            }
            return max;
        }

        double MinOut1Value() {
            double min = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].out1;
                if (value < min) {
                    min = value;
                }
            }
            return min;
        }

        double AvgOut1Value() {
            double total = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].out1;
                total += abs(value);
            }
            return sample_size_ > 0 ? total / double(sample_size_) : 0;
        }

        double MaxVal1Value() {
            double max = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].val1;
                if (value > max) {
                    max = value;
                }
            }
            return max;
        }

        double MinVal1Value() {
            double min = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].val1;
                if (value < min) {
                    min = value;
                }
            }
            return min;
        }

        double AvgVal1Value() {
            double total = 0;
            for (int i = 0; i < sample_size_; i++) {
                double value = samples_[i].val1;
                total += abs(value);
            }
            return sample_size_ > 0 ? total / double(sample_size_) : 0;
        }

    private:
        TimestampCallback timestamp_callback_;
        uint64_t counter_ = 0;
        uint64_t sample_size_;
        std::vector<PerfSample> samples_;
    };
}


#endif //OL_DSP_PROFILE_H
