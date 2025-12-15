#pragma once

template <typename FloatType>
struct MathConstants
{
    static constexpr FloatType tau = static_cast<FloatType> (6.283185307179586476L);
};

template <typename FloatType>
static FloatType ncos (size_t order, size_t i, size_t size) noexcept
{
    if (size <= 1)
    {
        return static_cast<FloatType>(1);
    }
    return std::cos (static_cast<FloatType>(order) * static_cast<FloatType>(i) * MathConstants<FloatType>::tau / static_cast<FloatType> (size - 1));
}

template <typename FloatType>
void calculateBlackmanHarris (FloatType* samples, size_t size) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        auto cos1 = ncos<FloatType>(1, i, size);
        auto cos2 = ncos<FloatType>(2, i, size);
        auto cos3 = ncos<FloatType>(3, i, size);
        samples[i] = static_cast<FloatType> (0.35875 - 0.48829 * cos1 + 0.14128 * cos2 - 0.01168 * cos3);
    }
}
