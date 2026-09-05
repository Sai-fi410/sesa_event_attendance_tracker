#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Participant Struct (Extending reference code structure)
struct Participant {
    string collegeId;
    string name;
    string email;
    string phone;
    string branch;
    string year;
    bool isPresent;
    string checkInTime;
};

// Global database in memory (persisted in CSV file)
vector<Participant> participants;
const string DATA_FILE = "data/participants.csv";

// Helper: Trim whitespace
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Helper: Case-insensitive search match
string toLower(const string& str) {
    string lowerStr = str;
    transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

// Helper: Get Current Timestamp String
string getCurrentTimestamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buf[32];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
            1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    return string(buf);
}

// Helper: Escape JSON special characters
string jsonEscape(const string& input) {
    string output = "";
    for (char c : input) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\b') output += "\\b";
        else if (c == '\f') output += "\\f";
        else if (c == '\n') output += "\\n";
        else if (c == '\r') output += "\\r";
        else if (c == '\t') output += "\\t";
        else output += c;
    }
    return output;
}

// URL Decode helper
string urlDecode(const string& str) {
    string ret;
    char ch;
    int i, ii;
    for (i = 0; i < (int)str.length(); i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i += 2;
        } else if (str[i] == '+') {
            ret += ' ';
        } else {
            ret += str[i];
        }
    }
    return ret;
}

// Load participants from CSV
void loadParticipants() {
    participants.clear();
    ifstream file(DATA_FILE);
    if (!file.is_open()) {
        cout << "[Data] CSV file not found. Creating default storage: " << DATA_FILE << endl;
        return;
    }

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (isHeader) {
            isHeader = false; // Skip CSV header row
            continue;
        }

        stringstream ss(line);
        string id, name, email, phone, branch, year, presentStr, checkIn;
        
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, email, ',');
        getline(ss, phone, ',');
        getline(ss, branch, ',');
        getline(ss, year, ',');
        getline(ss, presentStr, ',');
        getline(ss, checkIn, ',');

        Participant p;
        p.collegeId = trim(id);
        p.name = trim(name);
        p.email = trim(email);
        p.phone = trim(phone);
        p.branch = trim(branch);
        p.year = trim(year);
        p.isPresent = (trim(presentStr) == "1" || toLower(trim(presentStr)) == "true");
        p.checkInTime = trim(checkIn);

        if (!p.collegeId.empty()) {
            participants.push_back(p);
        }
    }
    file.close();
    cout << "[Data] Loaded " << participants.size() << " participant records from " << DATA_FILE << endl;
}

// Save participants to CSV
void saveParticipants() {
    ofstream file(DATA_FILE);
    if (!file.is_open()) {
        cerr << "[Error] Could not open file for writing: " << DATA_FILE << endl;
        return;
    }

    file << "CollegeID,Name,Email,Phone,Branch,Year,IsPresent,CheckInTime\n";
    for (const auto& p : participants) {
        file << p.collegeId << ","
             << p.name << ","
             << p.email << ","
             << p.phone << ","
             << p.branch << ","
             << p.year << ","
             << (p.isPresent ? "1" : "0") << ","
             << p.checkInTime << "\n";
    }
    file.close();
    cout << "[Data] Saved " << participants.size() << " records to CSV." << endl;
}

// Convert single participant to JSON string
string participantToJSON(const Participant& p) {
    stringstream ss;
    ss << "{"
       << "\"collegeId\":\"" << jsonEscape(p.collegeId) << "\","
       << "\"name\":\"" << jsonEscape(p.name) << "\","
       << "\"email\":\"" << jsonEscape(p.email) << "\","
       << "\"phone\":\"" << jsonEscape(p.phone) << "\","
       << "\"branch\":\"" << jsonEscape(p.branch) << "\","
       << "\"year\":\"" << jsonEscape(p.year) << "\","
       << "\"isPresent\":" << (p.isPresent ? "true" : "false") << ","
       << "\"checkInTime\":\"" << jsonEscape(p.checkInTime) << "\""
       << "}";
    return ss.str();
}

// Convert all participants to JSON array
string getParticipantsJSON() {
    stringstream ss;
    ss << "[";
    for (size_t i = 0; i < participants.size(); ++i) {
        ss << participantToJSON(participants[i]);
        if (i + 1 < participants.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

// Calculate and format statistics to JSON
string getStatsJSON() {
    int totalRegistered = participants.size();
    int totalPresent = 0;
    
    map<string, int> yearTotal;
    map<string, int> yearPresent;
    map<string, int> branchTotal;
    map<string, int> branchPresent;

    for (const auto& p : participants) {
        if (p.isPresent) totalPresent++;
        
        yearTotal[p.year]++;
        if (p.isPresent) yearPresent[p.year]++;

        branchTotal[p.branch]++;
        if (p.isPresent) branchPresent[p.branch]++;
    }

    int totalAbsent = totalRegistered - totalPresent;
    double percentage = (totalRegistered > 0) ? ((double)totalPresent / totalRegistered) * 100.0 : 0.0;

    stringstream ss;
    ss << "{"
       << "\"totalRegistered\":" << totalRegistered << ","
       << "\"totalPresent\":" << totalPresent << ","
       << "\"totalAbsent\":" << totalAbsent << ","
       << "\"attendancePercentage\":" << percentage << ",";

    // Year breakdown
    ss << "\"yearStats\":{";
    size_t count = 0;
    for (const auto& entry : yearTotal) {
        string yr = entry.first;
        int tot = entry.second;
        int pres = yearPresent[yr];
        ss << "\"" << jsonEscape(yr) << "\":{\"total\":" << tot << ",\"present\":" << pres << "}";
        if (++count < yearTotal.size()) ss << ",";
    }
    ss << "},";

    // Branch breakdown
    ss << "\"branchStats\":{";
    count = 0;
    for (const auto& entry : branchTotal) {
        string br = entry.first;
        int tot = entry.second;
        int pres = branchPresent[br];
        ss << "\"" << jsonEscape(br) << "\":{\"total\":" << tot << ",\"present\":" << pres << "}";
        if (++count < branchTotal.size()) ss << ",";
    }
    ss << "}";

    ss << "}";
    return ss.str();
}

// Extract JSON field string or raw value helper (resilient parsing)
string extractJsonField(const string& body, const string& fieldName) {
    if (body.empty()) return "";

    string lowerBody = toLower(body);
    string lowerKey = toLower(fieldName);

    size_t pos = lowerBody.find(lowerKey);
    if (pos == string::npos) return "";

    pos = body.find_first_of(":=", pos);
    if (pos == string::npos) return "";

    size_t start = pos + 1;
    while (start < body.length() && (body[start] == ' ' || body[start] == '"' || body[start] == '\'' || body[start] == '\\')) {
        start++;
    }

    size_t end = start;
    while (end < body.length() && body[end] != '"' && body[end] != '\'' && body[end] != ',' && body[end] != '}' && body[end] != '&' && body[end] != '\r' && body[end] != '\n' && body[end] != '\\') {
        end++;
    }

    return urlDecode(trim(body.substr(start, end - start)));
}

// Read text file content for serving frontend assets
string readFile(const string& filepath) {
    ifstream file(filepath, ios::binary);
    if (!file.is_open()) return "";
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Main HTTP Client Connection Handler
void handleClient(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    string requestStr(buffer, bytesReceived);

    stringstream requestStream(requestStr);
    string method, path, httpVersion;
    requestStream >> method >> path >> httpVersion;

    // Check Content-Length header for POST requests
    size_t contentLength = 0;
    size_t clPos = requestStr.find("Content-Length:");
    if (clPos == string::npos) clPos = requestStr.find("content-length:");
    if (clPos != string::npos) {
        size_t clEnd = requestStr.find("\r\n", clPos);
        if (clEnd != string::npos) {
            string clVal = requestStr.substr(clPos + 15, clEnd - (clPos + 15));
            contentLength = atoi(trim(clVal).c_str());
        }
    }

    // Separate HTTP Headers and Body
    string body = "";
    size_t headerEnd = requestStr.find("\r\n\r\n");
    if (headerEnd != string::npos) {
        body = requestStr.substr(headerEnd + 4);
    }

    // Read remaining body payload if needed
    while (body.length() < contentLength) {
        int extra = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (extra <= 0) break;
        body.append(buffer, extra);
    }

    string statusLine = "HTTP/1.1 200 OK\r\n";
    string contentType = "text/html; charset=UTF-8";
    string responseBody = "";

    // ROUTING HANDLER
    if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            responseBody = readFile("index.html");
            contentType = "text/html; charset=UTF-8";
        } else if (path == "/styles.css") {
            responseBody = readFile("styles.css");
            contentType = "text/css; charset=UTF-8";
        } else if (path == "/app.js") {
            responseBody = readFile("app.js");
            contentType = "application/javascript; charset=UTF-8";
        } else if (path == "/api/participants") {
            responseBody = getParticipantsJSON();
            contentType = "application/json; charset=UTF-8";
        } else if (path == "/api/stats") {
            responseBody = getStatsJSON();
            contentType = "application/json; charset=UTF-8";
        } else {
            statusLine = "HTTP/1.1 404 Not Found\r\n";
            responseBody = "{\"error\":\"Resource Not Found\"}";
            contentType = "application/json";
        }
    } else if (method == "POST") {
        if (path == "/api/mark") {
            string searchKey = extractJsonField(body, "query");
            if (searchKey.empty()) {
                searchKey = extractJsonField(body, "id");
            }
            searchKey = trim(toLower(searchKey));

            bool found = false;
            Participant* matchedStudent = nullptr;

            for (auto& p : participants) {
                if (toLower(p.collegeId) == searchKey ||
                    toLower(p.email) == searchKey ||
                    toLower(p.name) == searchKey ||
                    p.phone == searchKey) {
                    p.isPresent = true;
                    p.checkInTime = getCurrentTimestamp();
                    matchedStudent = &p;
                    found = true;
                    break;
                }
            }

            if (found && matchedStudent != nullptr) {
                saveParticipants(); // Save update to CSV
                stringstream ss;
                ss << "{\"success\":true,\"message\":\"Attendance marked present successfully!\",\"participant\":" 
                   << participantToJSON(*matchedStudent) << "}";
                responseBody = ss.str();
            } else {
                statusLine = "HTTP/1.1 404 Not Found\r\n";
                responseBody = "{\"success\":false,\"message\":\"Participant not found in registration database.\"}";
            }
            contentType = "application/json; charset=UTF-8";
        } else if (path == "/api/add") {
            Participant p;
            p.collegeId = trim(extractJsonField(body, "collegeId"));
            p.name = trim(extractJsonField(body, "name"));
            p.email = trim(extractJsonField(body, "email"));
            p.phone = trim(extractJsonField(body, "phone"));
            p.branch = trim(extractJsonField(body, "branch"));
            p.year = trim(extractJsonField(body, "year"));
            p.isPresent = false;
            p.checkInTime = "";

            if (!p.collegeId.empty() && !p.name.empty()) {
                participants.push_back(p);
                saveParticipants();
                stringstream ss;
                ss << "{\"success\":true,\"message\":\"New participant registered successfully.\",\"participant\":" 
                   << participantToJSON(p) << "}";
                responseBody = ss.str();
            } else {
                statusLine = "HTTP/1.1 400 Bad Request\r\n";
                responseBody = "{\"success\":false,\"message\":\"College ID and Name are required fields.\"}";
            }
            contentType = "application/json; charset=UTF-8";
        } else if (path == "/api/reset") {
            for (auto& p : participants) {
                p.isPresent = false;
                p.checkInTime = "";
            }
            saveParticipants();
            responseBody = "{\"success\":true,\"message\":\"Attendance status reset to absent for all records.\"}";
            contentType = "application/json; charset=UTF-8";
        } else {
            statusLine = "HTTP/1.1 404 Not Found\r\n";
            responseBody = "{\"error\":\"Unknown API Endpoint\"}";
            contentType = "application/json";
        }
    }

    stringstream responseHeader;
    responseHeader << statusLine
                   << "Content-Type: " << contentType << "\r\n"
                   << "Content-Length: " << responseBody.size() << "\r\n"
                   << "Access-Control-Allow-Origin: *\r\n"
                   << "Connection: close\r\n\r\n";

    string fullResponse = responseHeader.str() + responseBody;
    send(clientSocket, fullResponse.c_str(), fullResponse.size(), 0);
    closesocket(clientSocket);
}

int main() {
    cout << "===============================================" << endl;
    cout << "   COLLEGE EVENT ATTENDANCE TRACKER - BACKEND   " << endl;
    cout << "===============================================" << endl;

    // Load database from CSV file
    loadParticipants();

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[Error] WSAStartup failed." << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "[Error] Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    // Set socket SO_REUSEADDR
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[Error] Bind failed on port 8080." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "[Error] Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "\n[Server] C++ Server running at http://localhost:8080" << endl;
    cout << "[Server] Open http://localhost:8080 in your browser to access Attendance Tracker.\n" << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
