#include <iostream>
#include <string>
#include <locale.h>
#include <vector>



#include "gui_backend.h"
#include "utilities.h"
#include "classes.h"

using namespace std;

class myApp : public App<myApp> {
public:
    const string stationsUrl = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    const string sensorsUrlRoot = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/"; //append stationId before using
    const string readingUrlRoot = "https://api.gios.gov.pl/pjp-api/rest/data/getData/"; //append sensorId before using

    const char* errorMessage = "No errors yet"; //latest error will always be stored here
    bool online = false; //this determines whether data is loaded from file or directly from API request

    
    //Station currentStation;
    //Sensor currentSensor;

    //listofstationNamesCombo_cstr
    
    //jsonValue stationInfo
    //listofSesors cstr
    //jsonValue sensorInfo

    //jsonValue readings 
    //vector dates
    //vector values

    enum class AppState {
        STATION_MENU,
        SENSOR_MENU,
        READING_VIEW,
        INVALID_STATE
    };

    AppState appState = AppState::STATION_MENU;

    string stationListJson;
    StationList currentStationList;

    string sensorListJson;
    SensorList currentSensorList;

    string readingJson;
    Reading currentReading;

    myApp() = default;
    ~myApp() = default;

    void StartUp() {
        if (performCurlRequest(stationsUrl, stationListJson)) {
            online = true;
        }  
        else {
            if (loadStationsOffline(stationListJson)) {
                online = false;
                //create station list object
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

void myApp::showStationMenu() { //based on the below, station list must prepare the ids for further loads, and prepare json value to save station data to file
    
    if (ImGui::Combo("Choose a station.", &currentStationList.index, currentStationList.c_strNames.data(), currentStationList.c_strNames.size())) {
        //if something is chosen
        //station list prepares json value in case of save
        //cerr << "requesting" << currentStationList.ids[currentStationList.index] << endl;
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
            //loads sensors from file, creates object
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
                currentReading = Reading(readingJson);
                appState = AppState::READING_VIEW;
            }
            else {
                appState = AppState::INVALID_STATE;
            }
        }
        else {
            //load readings from file
        }
    }
}

void myApp::showReadingScreen() {
    if (ImPlot::BeginPlot("Reading from sensor")) {
        ImPlot::SetupAxes("Time", currentReading.paramName.c_str());
        
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisFormat(ImAxis_X1, "%H:%M\n%d-%m");
    
        ImPlot::PlotLine(currentReading.paramName.c_str(),
                         currentReading.dateTimestamps.data(),
                         currentReading.values.data(),
                         static_cast<int>(currentReading.values.size()));
    
        ImPlot::EndPlot();
    }
    
    if (online) {
        if (ImGui::Button("Save")) {
            ImGui::Text("(¬.¬)");
        }
    }
}

