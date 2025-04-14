#include <iostream>
#include <string>
#include <locale.h>
#include <vector>


#include <curl/curl.h>
#include <json/json.h>

#include "app.h"
#include "utilities.h"
#include "gui_utilities.h"

using namespace std;

class myApp : public App<myApp> {
public:
    std::string latestErrorMessage;
    
    //client variables related to Station selection
    bool gotStationList;
    std::vector<Station> stationList;
    std::vector<const char*> stationNamesCStr;
    int selectedStationIndex = -1;
    Station selectedStation;

    //client variables related to Sensor selection
    bool loadedSensors = false;
    std::vector<const char*> sensorNamesCStr;
    int selectedSensorIndex = -1;
    Sensor selectedSensor;



    bool sensorSelected = false;

    std::vector<Reading> latestReading;
    bool readingLoaded = false;
    bool showRawReading = false;

    myApp() = default;
    ~myApp() = default;

    void StartUp() {
        //getting the initial list of stations for a selection screen
        std::optional<vector<Station>> successfullLookup = stationLookup();
    
        if (successfullLookup) {
            stationList = *successfullLookup;
            gotStationList = true;

            stationNamesCStr.clear();
            for (const auto& station : stationList) {
                stationNamesCStr.push_back(station.stationName.c_str());
            }
        }
        else {
            latestErrorMessage = "Station Lookup failed!";
            gotStationList = false;
        }
    }

    void Update() {
        ImVec2 viewportSize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(viewportSize);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("FullScreenWindow", NULL, //create window spanning the entire viewport
            ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
    
        
        if (gotStationList) {
            ImGui::Text("Choose a station to monitor.");


            if (ImGui::Combo("##Choose Station", &selectedStationIndex, stationNamesCStr.data(), stationNamesCStr.size())) {
                selectedStation = stationList[selectedStationIndex];
                loadedSensors = false;
                sensorSelected = false;
                readingLoaded = false;
            }
            showStationSaveButton();
            ImGui::Separator();
            if (selectedStationIndex != -1) {
                ImGui::Text(("Selected station: " + selectedStation.stationName).c_str());
                if (ImGui::Button("Load sensors for selected station")) {
                    if (selectedStation.loadSensors()) {
                        loadedSensors = true;
                        sensorSelected = false;
                        readingLoaded = false;
                        selectedSensorIndex = -1;

                        sensorNamesCStr.clear();
                        for (const auto& sensor : selectedStation.mySensors) {
                            sensorNamesCStr.push_back(sensor.paramName.c_str());
                        }
                    }
                    else {
                        latestErrorMessage = "Couldn't load sensors";
                        sensorNamesCStr.clear();
                        loadedSensors = false;
                        sensorSelected = false;
                        readingLoaded = false;
                    }
                }
                if (loadedSensors) {
                    if (selectedStation.mySensors.empty()) {
                        ImGui::Text("No sensors available for the selected station. Try Loading them.");
                    } else {
                        if (ImGui::Combo("##Choose Sensor", &selectedSensorIndex, sensorNamesCStr.data(), sensorNamesCStr.size())) {
                            selectedSensor = selectedStation.mySensors[selectedSensorIndex];
                            sensorSelected = true;
                        }
                        
                        if (ImGui::Button("Save Sensor List to file")) {
                            selectedStation.updateSensorList();
                        }

                        if (sensorSelected) {
                            if (ImGui::Button("Get reading from sensor.")) {
                                if (selectedSensor.getReading()) {
                                    latestReading = selectedSensor.latestReading;
                                    readingLoaded = true;
                                }
                                else {
                                    latestErrorMessage = "Couldn't get reading from API.";
                                }   
                            }
                        }
                    }
                }
            }
            else {
                ImGui::Text(latestErrorMessage.c_str());
            }
        }
        else {
            ImGui::Text(latestErrorMessage.c_str());
        }
        
        ImGui::End();

        if (readingLoaded) {
            ImGui::Begin("Readings from current sensor");
            if (!latestReading.empty()) {
                std::vector<double> x_data;
                std::vector<double> y_data;
        
                for (size_t i = latestReading.size() - 1; i > 0 ; --i) {
                    if (static_cast<double>(latestReading[i].value) != 0) {
                        x_data.push_back(static_cast<double>(i));
                        y_data.push_back(static_cast<double>(latestReading[i].value));
                    }
                }
        
                if (ImPlot::BeginPlot("Air Quality Plot")) {
                    ImPlot::PlotLine("Sensor Values", x_data.data(), y_data.data(), static_cast<int>(x_data.size()));
                    ImPlot::EndPlot();
                }
                ImGui::Text("Stats: ");
                ImGui::Text("Min: %f", min(y_data));
                ImGui::Text("Max: %f", max(y_data));
                ImGui::Text("Average: %f", average(y_data));

                ImGui::Separator();
                ImGui::Checkbox("Show raw data.", &showRawReading);

                if (showRawReading) {
                    for (const auto& reading : latestReading) {
                        ImGui::Text("Date: %s", reading.date.c_str());
                        ImGui::Text("Value: %d", reading.value);
                        ImGui::Separator();
                    
                    }
                }
            } else {
                ImGui::Text("No readings available.");
            }

        ImGui::End();
        }
    }
};


int main() {
    myApp app;
    app.Run();

    return 0;
}