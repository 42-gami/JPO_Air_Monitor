#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <locale.h>
#include <vector>
#include <optional>
#include <fstream>

#pragma once

using namespace std;

class Station;
class Sensor;
class Reading;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool performCurlRequest(const string& url, string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        return false;
    }

    return true;
}


bool parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot) {
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(jsonResponse, parsedRoot);

    if (!parsingSuccessful) {
        cerr << "Failed to parse JSON: " << endl;
        return false;
    }

    return true;
}




class Station {
public:
    string id;
    string stationName;
    double gegrLat;
    double gegrLon;
    vector<Sensor> mySensors;
    
    Station(string id = "undefined", string stationName = "undefined", double gegrLat = 0, double gegrLon = 0, vector<Sensor> mySensors = {}) 
    : id(id), stationName(stationName), gegrLat(gegrLat), gegrLon(gegrLon), mySensors(mySensors){}

        
    bool loadParams(string desiredId);

    bool loadSensors();

    bool updateSensorList(string pathToFile);
    bool loadSensorsOffline();
};


    
class Sensor {
public:
    string id;
    string paramName;
    vector<Reading> latestReading;
    
    Sensor(string id = "undefined", string paramName = "undefined", vector<Reading> latestReading = {}) : id(id), paramName(paramName), latestReading(latestReading) {};

    bool getReading();  
    
    //bool getReadingOffline();
};
    
class Reading {
public:
    string date;
    int value;

    Reading(string date, double value) : date(date), value(value) {}
};

/// @brief This function is used to get a list of Station objects. It leaves the sensor vector empty - it needs to be loaded later using loadSensors().
/// @return object of class optional, that is the vector of Stations if successful, false if failed.
optional<vector<Station>> stationLookup() {
    string api_url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    string api_reply;
    vector<Station> stationList;
    if (performCurlRequest(api_url, api_reply)) {
        Json::Value root;
        if (parseJsonResponse(api_reply, root)) {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++)
            {
                Json::Value stationData = *it;
                string currentId = stationData["id"].asString();
                string currentStationName = stationData["stationName"].asString();
                double currentGegrLat = stod(stationData["gegrLat"].asString());
                double currentGegrLon = stod(stationData["gegrLon"].asString());

                Station currentStation = Station(currentId, currentStationName, currentGegrLat, currentGegrLon);
                stationList.push_back(currentStation);
            }
            return stationList;
        }
    }
    
    return nullopt; //false
}

/// @brief Function used to update the savefile for stationLookUpOffline
/// @return true if successfull, false if failed
bool updateStationList(string filePath = "saves/stations.json") {
    string api_url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    string api_reply;
    if (performCurlRequest(api_url, api_reply)) {
        ofstream file(filePath);

        if (!file) {
            return false;
        }
        
        file << api_reply;
        file.close();
        return true;
    }
    return false;
}
/*
/// @brief Same as stationLookup but uses a json savefile instead of an API call
/// @param datafile 
optional<vector<Station>> stationLookupOffline(string datafile = "saves/stations.json") {
        vector<Station> stationList;
        if (parseJsonResponse(api_reply, root)) {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++)
            {
                Json::Value stationData = *it;
                string currentId = stationData["id"].asString();
                string currentStationName = stationData["stationName"].asString();
                double currentGegrLat = stod(stationData["gegrLat"].asString());
                double currentGegrLon = stod(stationData["gegrLon"].asString());

                Station currentStation = Station(currentId, currentStationName, currentGegrLat, currentGegrLon);
                stationList.push_back(currentStation);
            }
            return stationList;
        }
    return nullopt; //false
}
*/

/// @brief This function is used to load up the list of sensors into a Station object. It will clear the current sensor list. Must be used on Stations acquired through stationLookup()
/// @return true if successful, false if the API request or parsing fails.
bool Station::loadSensors() {
    mySensors.clear();
    string api_url = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + id;
    string api_reply;
    if (performCurlRequest(api_url, api_reply)) {
        Json::Value root;
        if (parseJsonResponse(api_reply, root)) {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++)
            {
                Json::Value sensorData = *it;
                string currentId = sensorData["id"].asString();
                string currentParamName = sensorData["param"]["paramName"].asString();
                Sensor currentSensor = Sensor(currentId, currentParamName);
                mySensors.push_back(currentSensor);
            }
            return true;
        }
    }
    return false;
}

bool Station::updateSensorList(string pathToFile = "saves/sensors/") {
    string api_url = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + id;
    string api_reply;

    string filePath = pathToFile + "s" + id + ".json";

    if (performCurlRequest(api_url, api_reply)) {
        ofstream file(filePath);

        if (!file) {
            return false;
        }
        
        file << api_reply;
        file.close();
        return true;
    }
    return false;
}

/// @brief Same as load Sensors but from a json savefile, if it exists.
bool Station::loadSensorsOffline() {
    return false;
} 

/// @brief This function loads up the latestReading attribute of the Sensor object with date - value pairs.
/// @return true if successful, false if API request or parsing fails.
bool Sensor::getReading() {
    latestReading.clear();
    string api_url = "https://api.gios.gov.pl/pjp-api/rest/data/getData/" + id;
    string api_reply;
    if (performCurlRequest(api_url, api_reply)) {
        Json::Value outerRoot;
        if (parseJsonResponse(api_reply, outerRoot)) {
            Json::Value root = outerRoot["values"];
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++)
            {
                Json::Value readingData = *it;
                string currentDate = readingData["date"].asString();
                double currentValue = readingData["value"].asDouble();
                Reading currentReading = Reading(currentDate, currentValue);
                latestReading.push_back(currentReading);
            }
            return true;
        }
    }

    return false;
}