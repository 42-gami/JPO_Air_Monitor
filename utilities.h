#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <unistd.h>

#include <curl/curl.h>
#include <json/json.h>


namespace fs = std::filesystem;
using namespace std;

/// @brief Function specifying the format of the output of a Curl request
/// @return 
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/// @brief This function is used for all Curl requests in the project
/// @param url url of API request
/// @param response the response
/// @return true if successful
bool performCurlRequest(const string& url, string& response) {
    response.clear();
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

/// @brief Function used for parsing all Json in the project
/// @param jsonResponse string to parse
/// @param parsedRoot Json::Value object for later manipulation
/// @return true if successful
bool parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot) {
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(jsonResponse, parsedRoot);

    if (!parsingSuccessful) {
        cerr << "Failed to parse JSON: " << endl;
        return false;
    }

    return true;
}

/// @brief Used to generate vector of station names, mainly to display in the GUI. The indexing order of the names is important for functionality.
/// @param jsonString string with Json formatting, ideally obtained by API request or read from file
/// @return optional object thats a vector of names if function is successful or false if something fails
optional<vector<string>> generateStationList(const string& jsonString) {
    vector<string> stationList;
    Json::Value root;
    if (parseJsonResponse(jsonString, root)) {
        for (const auto& stationData : root) {
            if (stationData.isMember("stationName")) {
                stationList.push_back(stationData["stationName"].asString());
            }
        }
        return stationList;
    } else {
        return nullopt;
    }
}

/// @brief Used for getting a Json-formatted string for a particular station from generateStationList()
/// @param stationList Json-formatted string of station data
/// @param index index of station we want to obtain
/// @return Json-formatted string to be loaded into a function
string getStationDataString(string stationList, int index) {
    string stationData = "";
    Json::Value root;
    parseJsonResponse(stationList, root);
    Json::FastWriter fastWriter;
    stationData = fastWriter.write(root[index]);
    return stationData;
}

/// @brief Used to load list of stations from file instead of 
/// @param response passed by reference. The string from file will get saved here
/// @param filePath Path to json file with stations
/// @return true if successful
bool loadStationsOffline(string& response, string filePath = "") {
    return false;
}

string getRootLinux() {
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    
    if (len == -1) {
        return "";
    }
    
    buffer[len] = '\0';
    std::filesystem::path exePath(buffer);
    
    std::filesystem::path rootPath = exePath.parent_path().parent_path();
    
    return rootPath.string();
}

void writeToFile(const std::string& filePath, const std::string& content) {
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << content;
        outFile.close();
    } else {
        std::cerr << "Unable to open file for writing: " << filePath << "\n";
    }
}

std::string readStringFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}