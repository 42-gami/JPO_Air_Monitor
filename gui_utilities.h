#include "utilities.h"
#include <algorithm>
#include <numeric>

/// @brief Function used to get a vector of station names to display in ImGui.
std::vector<const char*> generateListOfNames(std::vector<Station>& stationList) {
    std::vector<std::string> stationNamesStr;
    std::vector<const char*> stationNamesCStr;

    for (const auto& station : stationList) {
        stationNamesStr.push_back(station.stationName);
        stationNamesCStr.push_back(stationNamesStr.back().c_str());
    }

    return stationNamesCStr;
}

template <typename T>
T min(const std::vector<T>& vec) {
    return *min_element(vec.begin(), vec.end());
}

template <typename T>
T max(const std::vector<T>& vec) {
    return *max_element(vec.begin(), vec.end());
}

template <typename T>
T average(const std::vector<T>& vec) {
    T sum = std::accumulate(vec.begin(), vec.end(), T(0));
    return sum / static_cast<T>(vec.size());
}

