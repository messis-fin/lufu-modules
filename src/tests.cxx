
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "ClockDSP.hpp"
#include "WavWriter.hpp"
/*
TEST_CASE("continuous_iterator basics")
{

    std::vector<int> v{ 1,2,3,4,5,6,7,8,9,10 };
    continuous  _iterator<std::vector<int>> it(v);

    CHECK(*it == 1);

    it += 0.2;
    CHECK(*it == 1);

    it += 0.6;
    CHECK(*it == 1);

    it += 0.2;
    CHECK(*it == 2);

    it += 0.2;
    CHECK(*it == 2);

    it += -1.2;
    CHECK(*it == 1);

    it += -0.00001;
    CHECK(*it == 10);
}

*/

TEST_CASE("wavbasics")
{
    lufu::WavSink sink("tmp.wav", 44100);

    float f = 0;
    for (int j = 0; j < 15; j++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (int i = 0; i < 44100; i++)
        {
            auto df = 2 * 3.1415927 / 441;
            sink.push_samples(5.0 * sin(f), 2.0 * sin(2 * f));
            f += df;
            // std::this_thread::sleep_for(std::chrono::microseconds(230));
        }
    }
}
