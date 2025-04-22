#include <iostream>
#include <string>
#include <locale.h>
#include <vector>



#include "gui_backend.h"
#include "utilities.h"
#include "classes.h"

using namespace std;

/// \file gui_app.h
/// \brief Responsible for showing the UI, getting user input and coordinating everything else in the program


/// \mainpage Air Quality Monitor
/// Program used for monitoring air quality in Poland. It fetches data from the GIOS REST API and displays it in a user friendly format thanks to ImGui.

/// @brief Class responsible for presentation and coordination of all other elements of the program. 
class myApp : public App<myApp> {
public:

    /// \name Relevant Urls for the REST API
    /// These are the Urls used for making API requests. Sensors and readings must be appended with correct ids before using.
    /// @{
    const string stationsUrl = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    const string sensorsUrlRoot = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/"; //append stationId before using
    const string readingUrlRoot = "https://api.gios.gov.pl/pjp-api/rest/data/getData/"; //append sensorId before using
    
    const string geolocationUrl = "http://ip-api.com/json/";
    /// @}


    /// @brief  This loads up the project root path
    const string projectRoot = getRootLinux();
    
    /// latest error always stored in here
    const char* errorMessage = "No errors yet";

    ///Flag which determines whether json-formatted strings are acquired by API request or from file
    bool online = false;

    
    /// All states of the Gui. INVALID_STATE basically restarts the app.
    enum class AppState {
        STATION_MENU,
        SENSOR_MENU,
        READING_VIEW,
        INVALID_STATE
    };

    AppState appState = AppState::STATION_MENU;


    /// \name Objects which hold and automatically calculate all of the currently used information.
    /// Created from coresponding strings.
    ///
    string stationListJson;
    StationList currentStationList;

    string sensorListJson;
    SensorList currentSensorList;

    string readingJson;
    Reading currentReading;
    ///@}

    myApp() = default;
    ~myApp() = default;


    /// @brief mainly used to check if REST API is reachable. Switches app to Offline mode when there is problems with connection.
    void StartUp() {
        //cerr << projectRoot << endl;
        if (performCurlRequest(stationsUrl, stationListJson)) {
            online = true;
        }  
        else {
            string stationFilePath = projectRoot + "/saves/stations.json";
    
            if (std::filesystem::exists(stationFilePath)) {
                stationListJson = readStringFromFile(stationFilePath);
                online = false;
            }
            else {
                appState = AppState::INVALID_STATE;
                return;
            }
        }
        currentStationList = StationList(stationListJson);
        appState = AppState::STATION_MENU;
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

        
        if (ImGui::Button("Go Online")) {
            StartUp();
        }
        
        switch(appState) {
            case AppState::STATION_MENU: {
                showStationMenu();
                break;
            }
            case AppState::SENSOR_MENU: {
                showStationMenu();
                showSensorMenu();
                break;
            }
            case AppState::READING_VIEW: {
                showStationMenu();
                showSensorMenu();
                break;
            }
            case AppState::INVALID_STATE: {
                ImGui::Text(errorMessage);
                StartUp();
                break;
            }
            default: {
                appState = AppState::INVALID_STATE;
                break;
            }
        }

        ImGui::End();

        if (appState == AppState::READING_VIEW) {
            ImGui::Begin("Readings from sensor");
            showReadingScreen();
            ImGui::End();
        }
    }

    void showStationMenu();
    void showSensorMenu();
    void showReadingScreen();
};

void myApp::showStationMenu() {
    if (online) {
        if (ImGui::Button("Find station closest to you.")) {
            string locJson;
            double lat;
            double lon;
            if (performCurlRequest(geolocationUrl, locJson)) {
                Json::Value root;
                if (parseJsonResponse(locJson, root)) {
                    lat = root["lat"].asDouble();
                    lon = root["lon"].asDouble();
                    currentStationList.selectNearestStation(Gegr(lat, lon));
                    const string functionalUrl = sensorsUrlRoot + currentStationList.ids[currentStationList.index];
                    if (performCurlRequest(functionalUrl, sensorListJson)) {
                        currentSensorList = SensorList(sensorListJson);
                        appState = AppState::SENSOR_MENU;
                    }
                    else {
                        appState = AppState::INVALID_STATE;
                    }
                }
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
    }
    if (ImGui::Combo("Choose a station.", &currentStationList.index, currentStationList.c_strNames.data(), currentStationList.c_strNames.size())) {

        if (online) {
            const string functionalUrl = sensorsUrlRoot + currentStationList.ids[currentStationList.index];
            //cerr << functionalUrl << endl;
            if (performCurlRequest(functionalUrl, sensorListJson)) {
                currentSensorList = SensorList(sensorListJson);
                //cerr << currentSensorList.names[0] << endl;
                //cerr << sensorListJson << endl;
                appState = AppState::SENSOR_MENU;
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
        else {
            string sensorFilePath = projectRoot + "/saves/sensors/s" + currentStationList.ids[currentStationList.index] + ".json";
    
            if (std::filesystem::exists(sensorFilePath)) {
                sensorListJson = readStringFromFile(sensorFilePath);
                currentSensorList = SensorList(sensorListJson);
                appState = AppState::SENSOR_MENU;
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
        //set sensors array
        //next state
    }
}

void myApp::showSensorMenu() { //consider checking if currentSensorList is actually set up
    //show combo based on sensor list object
    //if something is chosen

    if (ImGui::Combo("Choose sensor", &currentSensorList.index, currentSensorList.c_strNames.data(), currentSensorList.c_strNames.size())) {
        if (online) {
            const string functionalUrl = readingUrlRoot + currentSensorList.ids[currentSensorList.index];
            if (performCurlRequest(functionalUrl, readingJson)) {
                //cerr << functionalUrl << endl;
                currentReading = Reading(readingJson, currentSensorList.ids[currentSensorList.index]);
                appState = AppState::READING_VIEW;
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
        else {
            string readingFilePath = projectRoot + "/saves/readings/r" + currentSensorList.ids[currentStationList.index] + ".json";
    
            if (std::filesystem::exists(readingFilePath)) {
                readingJson = readStringFromFile(readingFilePath);
                currentReading = Reading(readingJson, currentSensorList.ids[currentSensorList.index]);
                appState = AppState::READING_VIEW;
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
    }
}

void myApp::showReadingScreen() {
    if (ImPlot::BeginPlot("##Reading from sensor")) {
        static bool autoFitAxes = true;
        ImGui::Checkbox("Fit axes", &autoFitAxes);

        if (autoFitAxes) {
        ImPlot::SetupAxes("Time", currentReading.paramName.c_str(),
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            } else {
                ImPlot::SetupAxes("Time", currentReading.paramName.c_str());
            }
        
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisFormat(ImAxis_X1, "%H:%M\n%m-%d");
    
        ImPlot::PlotLine(currentReading.paramName.c_str(),
                         currentReading.dateTimestamps.data(),
                         currentReading.values.data(),
                         static_cast<int>(currentReading.values.size()));
        

        ImGui::Separator();
        ImGui::Text("Average: %.2f", currentReading.average);
        ImGui::Text("Max value: %.2f at %s", currentReading.max, currentReading.maxTime.c_str());
        ImGui::Text("Min value (non-zero): %.2f at %s", currentReading.min, currentReading.minTime.c_str());
        ImGui::Separator();
        ImPlot::EndPlot();
    }
    
    if (online) {
        if (ImGui::Button("Save")) {
            if (currentReading.saveData(projectRoot)) {
                if (currentSensorList.saveData(projectRoot)) {
                    currentStationList.saveData(projectRoot);
                }
            }
        }
    }
}

