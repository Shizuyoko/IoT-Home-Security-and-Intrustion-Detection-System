//header files
#include <iostream>
#include "httplib.h"
#include "json.hpp"
#include "sqlite3.h"
#include <fstream>
#include <ctime>
#include <filesystem>
#include <chrono>

//naming shortcut for json library
using json = nlohmann::json;

//pointer to database
sqlite3 * db;

const std::string UPLOAD_DIR = "C://Users//User//CLionProjects//IOTBackendTest//src//uploads//";

//creates database table if it doesn't exist
void createTable() {

    //variable that stores the following sql command
    const char * sql =
        "CREATE TABLE IF NOT EXISTS events ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "device_id TEXT,"
        "event_type TEXT,"
        "location TEXT,"
        "timestamp TEXT"
        ");";

    //variable to store error message if one occurs
    char * error;

    //runs the stored sql command on the database, if not ok then error has occured
    if (sqlite3_exec(db, sql, 0, 0, &error) != SQLITE_OK) {
        std::cout << "Error creating table: " << error << std::endl;

        //frees error memory allocated
        sqlite3_free(error);
    }
}

//store event in database function
void insertEvent(std::string device, std::string type,
                 std::string location, std::string time) {

    //create statement object used to run commands
    sqlite3_stmt * stmt;

    //sql command to insert new event
    const char * sql =
        "INSERT INTO events (device_id,event_type,location,timestamp)"
        " VALUES (?,?,?,?);";

    //prepare statement
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    //place values inside strings (device id, type, location and timestamp)
    sqlite3_bind_text(stmt, 1, device.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, location.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, time.c_str(), -1, SQLITE_STATIC);

    //these two run and close the sql statement
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

//delete event in database function using id
void deleteEvent(int id) {

    sqlite3_stmt * stmt;

    //delete event with specific id
    const char * sql = "DELETE FROM events WHERE id = ?;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);

    //delete event and close statement
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int main() {

    //opens database file (if not already created it will create this automatically)
    if (sqlite3_open("events.db", &db)) {
        std::cout << "Could not open database\n";
        return 1;
    }

    //create events table
    createTable();

    //create server
    httplib::Server server;

    server.set_default_headers({
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
    {"Access-Control-Allow-Headers", "Content-Type"}
    });

    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    //simple health check endpoint
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {

        json response;
        response["status"] = "server running";

        res.set_content(response.dump(), "application/json");
    });

    //POST event endpoint
    server.Post("/api/events/", [](const httplib::Request& req,
                                  httplib::Response& res) {

        try {

            //read incoming request
            auto data = json::parse(req.body);

            std::string device = data["device_id"];
            std::string type = data["event_type"];
            std::string location = data["location"];
            std::string time;
            if (data.contains("timestamp")) {
                time = data["timestamp"];
            }
            else {
                // generate timestamp automatically
                time = std::to_string(std::time(nullptr));
            }


            //store in database
            insertEvent(device, type, location, time);

            //success message
            json response;
            response["message"] = "event stored";

            res.set_content(response.dump(), "application/json");

        } catch (...) {

            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
        }
    });

    server.Post("/upload_image", [&](const httplib::Request& req,
                                  httplib::Response& res) {

     if (req.body.empty()) {
         res.status = 400;
         res.set_content("{\"error\":\"empty body\"}", "application/json");
         return;
     }

     std::string device = "unknown";
     if (req.has_param("device_id")) {
         device = req.get_param_value("device_id");
     }
        std::time_t now = std::time(nullptr);

        std::tm localTime;
        localtime_s(&localTime, &now);

        char buffer[64];

        std::strftime(
        buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &localTime);

        std::string just_filename =
        device + "_" +
        std::string(buffer) +
        ".jpg";

        std::string filename = UPLOAD_DIR + just_filename;

     std::cout << "Received image bytes: " << req.body.size() << std::endl;
     std::cout << "Saving to: " << filename << std::endl;

     std::ofstream ofs(filename, std::ios::binary);
     ofs.write(req.body.data(), req.body.size());
     ofs.close();

     json response;
        response["message"] = "image saved";
        response["file"] = just_filename;
        response["url"] = "/images/" + just_filename;

     res.set_content(response.dump(), "application/json");
 });

    server.Get("/api/images",
[](const httplib::Request&, httplib::Response& res) {

    json images = json::array();

    for (const auto& entry :
         std::filesystem::directory_iterator(UPLOAD_DIR)) {

        images.push_back(
            entry.path().filename().string()
        );
    }

    res.set_content(
        images.dump(),
        "application/json"
    );
});

    //GET pictures endpoint
    server.Get("/images/(.*)", [](const httplib::Request& req, httplib::Response& res) {

    std::string filename = req.matches[1];

        std::string path = UPLOAD_DIR + filename;

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        res.status = 404;
        res.set_content("File not found", "text/plain");
        return;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    res.set_content(content, "image/jpeg");
    });

    //GET events endpoint
    server.Get("/api/events/", [](const httplib::Request& req,
                                 httplib::Response& res) {

        sqlite3_stmt * stmt;

        //sql command to get events
        std::string sql = "SELECT * FROM events ORDER BY timestamp DESC";

        //checks for filters
        if (req.has_param("device_id")) {
            sql += " WHERE device_id = ?";
        }

        sql += ";";

        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

        if (req.has_param("device_id")) {
            std::string device = req.get_param_value("device_id");
            sqlite3_bind_text(stmt, 1, device.c_str(), -1, SQLITE_STATIC);
        }

        //creates json list to store results
        json events = json::array();

        //loops through each row gotten from database
        while (sqlite3_step(stmt) == SQLITE_ROW) {

            json e;

            e["id"] = sqlite3_column_int(stmt, 0);
            e["device_id"] = (const char*)sqlite3_column_text(stmt, 1);
            e["event_type"] = (const char*)sqlite3_column_text(stmt, 2);
            e["location"] = (const char*)sqlite3_column_text(stmt, 3);
            e["timestamp"] = (const char*)sqlite3_column_text(stmt, 4);

            //add each event to array
            events.push_back(e);
        }

        sqlite3_finalize(stmt);

        //send array list back to user
        res.set_content(events.dump(), "application/json");
    });

    //DELETE event endpoint
    server.Delete(R"(/api/events/(\d+))",
        [](const httplib::Request& req, httplib::Response& res) {

            //get event id
        int id = std::stoi(req.matches[1]);

            //delete event from database
        deleteEvent(id);

        res.set_content("{\"message\":\"event deleted\"}", "application/json");
    });

    std::cout << "Server running on port 8080\n";

    //starts server
    server.listen("0.0.0.0", 8080);

    //close database when server stops
    sqlite3_close(db);

    return 0;
}