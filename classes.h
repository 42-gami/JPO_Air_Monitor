#include <string>
#include <vector>

#include <json/json.h>

using namespace std;

//every object created from json-formatted string, creates various arrays for appropriate use of the string
//every object saves the data that may once be used to load data into itself

//stationlist
//created by string
//contains:
//string array of names
//cstrs for imgui display
//string array of ids to get for the next request from file or api
//json value array for saving stuff
//currently selected name
//save function saves station from json value array with index chosen, appends it correctly

//senorlist
//created by string
//string array of sensor names
//cstrs names for imgui
//index chosen
//json value array for saving stuff
//save function saves object from json value array with index of index chosen, appends correctly

//readings
//created by string from file or api, name of request determined from sensorList array of ids
//stores json value of readings in case of saving
//stores x vals and y vals for imgui display
//can calculate some stats based on y vals
//save function saves json value of readings to file, appends correctly


class StationList {
public:
    vector<string> names;
    vector<const char*> c_strNames;
    vector<string> ids;
    vector<Json::Value> jsonData;
    int index = -1;

    StationList() {}

    StationList(string jsonString) {
        if (loadData(jsonString)) {
            prepareCStrNames();
        }
    }

    bool loadData(string jsonString) {
        index = -1;
        names.clear();
        ids.clear();
        jsonData.clear();
        Json::Value root;
        if (parseJsonResponse(jsonString, root)) {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++) {
                Json::Value stationData = *it;
                names.push_back(stationData["stationName"].asString());
                ids.push_back(stationData["id"].asString());
                jsonData.push_back(stationData);
            }
            return true;
        }
        return false;
    }

    void prepareCStrNames() {
        c_strNames.clear();
        for (const auto& name : names) {
            c_strNames.push_back(name.c_str());
        }
    }
};

class SensorList {
public:
    string stationId;
    vector<string> names;
    vector<const char*>c_strNames;
    vector<string> ids;
    vector<Json::Value> jsonData;
    int index = -1;

    SensorList() {}

    SensorList(string jsonString) {
        if (loadData(jsonString)) {
            prepareCStrNames();
        }
    }

    bool loadData(string jsonString) {
        index = -1;
        names.clear();
        ids.clear();
        jsonData.clear();
        Json::Value root;
        if (parseJsonResponse(jsonString, root)) {
            stationId = root[0]["stationId"].asString();
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++) {
                Json::Value sensorData = *it;
                names.push_back(sensorData["param"]["paramName"].asString());
                ids.push_back(sensorData["id"].asString());
                jsonData.push_back(sensorData);
            }
            return true;
        }
        return false;
    }

    void prepareCStrNames() {
        c_strNames.clear();
        for (const auto& name : names) {
            c_strNames.push_back(name.c_str());
        }
    }
};

class Reading {
public:
    string paramName;
    vector<double> values;
    vector<double> indices;
    vector<string> dates;


    double average;
    double min;
    string minTime;
    double max;
    string maxTime;
    string trend;

    Reading() {};

    Reading(string jsonString) {
        if (loadData(jsonString)) {
            
        }
    }

    bool loadData(string jsonString) {
        values.clear();
        dates.clear();
        indices.clear();
        Json::Value outerRoot;
        if (parseJsonResponse(jsonString, outerRoot)) {
            paramName = outerRoot["key"].asString();
            Json::Value root = outerRoot["values"];
            double index = 0;
            for (int i = root.size() - 1; i >= 0; --i) {
                Json::Value readingData = root[i];
                values.push_back(readingData["value"].asDouble());
                dates.push_back(readingData["date"].asString());
                indices.push_back(index);
                index += 1.0;
            }
            return true;
        }
        return false;
    }
};