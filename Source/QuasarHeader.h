#pragma once

#include <cstdint>
#include <JuceHeader.h>
#include "Aaaa.h"

enum FilterType
{
	HighPass,
	HighShelf,
	LowPass,
	LowShelf,
	PeakFilter
};

namespace quasar
{
	namespace colours
	{
		const juce::Colour enabled {0xff7391ff};
		const juce::Colour disabled {0xff555555};
		const juce::Colour staticText {0xffd3d3d3};
		const juce::Colour labelBackground {0xff17171a};
		const juce::Colour audioSignal {0xff4d76ff};
	}
}
