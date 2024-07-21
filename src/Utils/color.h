# pragma once

#include <tuple>

namespace Tonton {
namespace Utils {

	std::tuple<int, int, int> HSVtoRGB(float H, float S, float V);

} // namespace Utils
} // namespace Tonton