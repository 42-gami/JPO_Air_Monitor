#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <json/json.h>


using namespace std;

class Gegr { //helper class for distance calculations
    public:
        double lat;
        double lon;
    
        Gegr(double lat, double lon) : lat(lat), lon(lon) {}
    
        double distance(Gegr loc) { //only works when surface is locally flat, like Poland
            double dlat = loc.lat - lat;
            double dlon = loc.lon - lon;
    
            return std::sqrt(dlat * dlat + dlon * dlon);
        }
};

/// @brief helper function for converting date strings into unix timestamps
double convertDateToTimestamp(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return -1;
    }
    time_t time = std::mktime(&tm);
    return static_cast<double>(time);
}

class StationList {
public:
    vector<string> names;
    vector<const char*> c_strNames;
    vector<string> ids;
    vector<Gegr> locations;
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
        locations.clear();
        jsonData.clear();
        Json::Value root;
        if (parseJsonResponse(jsonString, root)) {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); it++) {
                Json::Value stationData = *it;
                names.push_back(stationData["stationName"].asString());
                ids.push_back(stationData["id"].asString());
                locations.push_back(Gegr(std::stod(stationData["gegrLat"].asString()), std::stod(stationData["gegrLon"].asString())));
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

    void selectNearestStation(Gegr myLoc = Gegr(50.0, 16.0)) {
        index = -1;
        double minDist = std::numeric_limits<double>::infinity();
    
        for (size_t i = 0; i < locations.size(); ++i) {
            double dist = myLoc.distance(locations[i]);
    
            if (dist < minDist) {
                minDist = dist;
                index = i;
            }
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
    string paramId;
    string jsonDataString;
    vector<double> values;
    vector<double> indices;
    vector<string> dates;
    vector<double> dateTimestamps;
    vector<const char*> c_strNames;
    Json::Value jsonData;

    double average;
    double min;
    string minTime;
    double max;
    string maxTime;
    string trend;

    Reading() {};

    Reading(string jsonString, string id) {
        paramId = id;
        if (loadData(jsonString)) {
            prepareCStrNames();
            prepareTimestamps();
            findMax();
            findMin();
            findAverage();
        }
    }

    bool loadData(string jsonString) {
        jsonDataString = jsonString;
        values.clear();
        dates.clear();
        indices.clear();
        Json::Value outerRoot;
        if (parseJsonResponse(jsonString, outerRoot)) {
            paramName = outerRoot["key"].asString();
            Json::Value root = outerRoot["values"];
            jsonData = root;
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

    void prepareCStrNames() {
        c_strNames.clear();
        for (const auto& date : dates) {
            c_strNames.push_back(date.c_str());
        }
    }

    void prepareTimestamps() {
        dateTimestamps.clear();
        for (const std::string& dateStr : dates) {
            double ts = convertDateToTimestamp(dateStr);
            dateTimestamps.push_back(ts);
        }
    }

    void findMin() {
        double minValue = std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < values.size(); ++i) {
            if (values[i] > 0 && values[i] < minValue) {
                minValue = values[i];
                minTime = dates[i];
            }
        }
        min = minValue;
    }
    

    void findMax() {
        if (values.empty()) return;

        max = values[0];
        maxTime = dates[0];

        for (size_t i = 1; i < values.size(); ++i) {
            if (values[i] > max && values[i] != 0) {
                max = values[i];
                maxTime = dates[i];
            }
        }
    }

    void findAverage() {
        if (values.empty()) {
            average = 0.0;
            return;
        }

        double sum = 0.0;
        for (size_t i = 0; i < values.size(); ++i) {
            sum += values[i];
        }

        average = sum / values.size();
    }

    bool saveData(string projectRoot) {
        string filePath = projectRoot + "/saves/readings/r" + paramId + ".json";
        if (std::filesystem::exists(filePath)) {
            string jsonDataFile = readStringFromFile(filePath);
            Json::Value root;
            if (parseJsonResponse(jsonDataFile, root)) {
                return false;
            }
            else {
                return false;
            }
        } else {
            writeToFile(filePath, jsonDataString);
            return true;
        }
    }
};

