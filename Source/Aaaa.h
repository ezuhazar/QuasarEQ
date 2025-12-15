#pragma once

#include <cmath>
#include <vector>

namespace DSPMath
{
    constexpr long double tau_ld = 6.283185307179586476L;
}

template <typename FloatType>
class  WindowingFunction
{
public:
    static FloatType getCosineTerm (const size_t harmonicOrder, const size_t i, const size_t windowSize) noexcept
    {
        if (windowSize <= 1)
        {
            return static_cast<FloatType>(1);
        }
        return std::cos (static_cast<FloatType>(harmonicOrder) * static_cast<FloatType>(i) * static_cast<FloatType>(DSPMath::tau_ld) / static_cast<FloatType> (windowSize - 1));
    }
    void calculateBlackmanHarris (FloatType* outputData, const size_t numSamples) noexcept
    {
        const FloatType a0 = static_cast<FloatType>(0.35875L);
        const FloatType a1 = static_cast<FloatType>(0.48829L);
        const FloatType a2 = static_cast<FloatType>(0.14128L);
        const FloatType a3 = static_cast<FloatType>(0.01168L);
        for (size_t i = 0; i < numSamples; ++i)
        {
            const auto cos1 = getCosineTerm(1, i, numSamples);
            const auto cos2 = getCosineTerm(2, i, numSamples);
            const auto cos3 = getCosineTerm(3, i, numSamples);
            outputData[i] = a0 - a1 * cos1 + a2 * cos2 - a3 * cos3;
        }
        FloatType sum (0);
        for (size_t i = 0; i < numSamples; ++i)
        {
            sum += outputData[i];
        }
        if (sum <= static_cast<FloatType>(0))
        {
            return;
        }
        const auto factor = static_cast<FloatType> (numSamples) / sum;
        for (size_t i = 0; i < numSamples; ++i)
        {
            outputData[i] *= factor;
        }
    }
private:
    std::vector<FloatType> windowTable;
};
