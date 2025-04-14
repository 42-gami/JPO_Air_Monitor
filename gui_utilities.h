#include "utilities.h"
#include <algorithm>
#include <numeric>

#include "app.h"

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

void showStationSaveButton() {
    if (ImGui::Button("Save list of stations for offline use.")) {
        updateStationList();
    }
}