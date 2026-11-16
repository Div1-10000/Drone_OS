#include "Database.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace DroneOS {

Database::Database()
    : db(nullptr)
{
}

bool Database::initialize(
    const std::string& databasePath)
{
    const int result =
        sqlite3_open(
            databasePath.c_str(),
            &db
        );

    if (result != SQLITE_OK)
    {
        std::cerr
            << "Database connection failed: "
            << sqlite3_errmsg(db)
            << std::endl;

        if (db != nullptr)
            sqlite3_close(db);

        db = nullptr;

        return false;
    }

    std::cout
        << "Database connection: OK"
        << std::endl;

    if (!execute(
            "PRAGMA foreign_keys = ON;"))
    {
        return false;
    }

    return executeSchema();
}

bool Database::execute(
    const std::string& sql)
{
    char* errorMessage = nullptr;

    const int result =
        sqlite3_exec(
            db,
            sql.c_str(),
            nullptr,
            nullptr,
            &errorMessage
        );

    if (result != SQLITE_OK)
    {
        std::cerr
            << "SQL error: "
            << (
                errorMessage
                    ? errorMessage
                    : "Unknown error"
            )
            << std::endl;

        sqlite3_free(errorMessage);

        return false;
    }

    return true;
}

bool Database::executeSchema()
{
    std::ifstream schemaFile(
        "database/schema.sql"
    );

    if (!schemaFile.is_open())
    {
        std::cerr
            << "Failed to open "
               "database/schema.sql"
            << std::endl;

        return false;
    }

    std::stringstream buffer;

    buffer << schemaFile.rdbuf();

    if (!execute(buffer.str()))
        return false;

    std::cout
        << "Database schema: OK"
        << std::endl;

    return true;
}

int Database::getDroneId()
{
    int id = -1;

    const char* sql =
        "SELECT drone_id "
        "FROM drones "
        "WHERE name='DRONE-X1' "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return -1;
    }

    if (sqlite3_step(statement) ==
        SQLITE_ROW)
    {
        id = sqlite3_column_int(
            statement,
            0
        );
    }

    sqlite3_finalize(statement);

    return id;
}

int Database::getMissionId()
{
    int id = -1;

    const char* sql =
        "SELECT mission_id "
        "FROM missions "
        "WHERE mission_name="
        "'AUTONOMOUS SURVEY' "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return -1;
    }

    if (sqlite3_step(statement) ==
        SQLITE_ROW)
    {
        id = sqlite3_column_int(
            statement,
            0
        );
    }

    sqlite3_finalize(statement);

    return id;
}

bool Database::createInitialDrone()
{
    const char* sql =
        "INSERT OR IGNORE INTO drones (name, model, status, battery) VALUES "
        "('DRONE-X1','Autonomous Survey Drone','ONLINE',100.0),"
        "('DRONE-X2','Autonomous Survey Drone','ONLINE',97.0),"
        "('DRONE-X3','Autonomous Survey Drone','ONLINE',94.0),"
        "('DRONE-X4','Autonomous Survey Drone','ONLINE',91.0),"
        "('DRONE-X5','Autonomous Survey Drone','ONLINE',88.0);";

    if (!execute(sql))
        return false;

    const char* telemetrySeed =
        "INSERT INTO telemetry(drone_id,altitude,speed,battery,signal,temperature,latitude,longitude) "
        "SELECT d.drone_id,120.0,0.0,d.battery,96.0,32.0,"
        "28.6139 + (d.drone_id-1)*0.00035,77.2090 + (d.drone_id-1)*0.00030 "
        "FROM drones d WHERE NOT EXISTS(SELECT 1 FROM telemetry t WHERE t.drone_id=d.drone_id);";
    if (!execute(telemetrySeed))
        return false;

    std::cout << "Fleet records (5 drones): OK" << std::endl;
    return true;
}

bool Database::createInitialMission()
{
    const char* sql =
        "INSERT INTO missions (drone_id, mission_name, status, progress) "
        "SELECT d.drone_id, 'AUTONOMOUS SURVEY', 'PLANNED', 0.0 "
        "FROM drones d "
        "WHERE NOT EXISTS ("
        " SELECT 1 FROM missions m "
        " WHERE m.drone_id=d.drone_id AND m.mission_name='AUTONOMOUS SURVEY'"
        ");";

    if (!execute(sql))
        return false;

    std::cout << "Fleet mission records: OK" << std::endl;
    return true;
}

std::string Database::getDashboardRecordsJson()
{
    auto escape = [](const unsigned char* raw) {
        std::string value;
        if (!raw) return value;
        for (const unsigned char* p = raw; *p; ++p) {
            switch (*p) {
                case '"': value += "\\\""; break;
                case '\\': value += "\\\\"; break;
                case '\n': value += "\\n"; break;
                case '\r': value += "\\r"; break;
                case '\t': value += "\\t"; break;
                default: value += static_cast<char>(*p); break;
            }
        }
        return value;
    };
    auto countRows = [this](const char* sql) {
        sqlite3_stmt* q = nullptr;
        int count = 0;
        if (sqlite3_prepare_v2(db, sql, -1, &q, nullptr) == SQLITE_OK &&
            sqlite3_step(q) == SQLITE_ROW) count = sqlite3_column_int(q, 0);
        if (q) sqlite3_finalize(q);
        return count;
    };

    std::ostringstream out;
    out << "{\"drones\":[";
    sqlite3_stmt* stmt = nullptr;
    const char* droneSql =
        "SELECT d.drone_id,d.name,d.model,d.status,COALESCE(t.battery,d.battery),"
        "COALESCE(t.altitude,0),COALESCE(t.speed,0),"
        "COALESCE(t.latitude,0),COALESCE(t.longitude,0),"
        "COALESCE(m.mission_name,'None'),COALESCE(m.status,'IDLE'),"
        "(SELECT COUNT(*) FROM telemetry tx WHERE tx.drone_id=d.drone_id) "
        "FROM drones d "
        "LEFT JOIN telemetry t ON t.telemetry_id=("
        " SELECT telemetry_id FROM telemetry WHERE drone_id=d.drone_id "
        " ORDER BY recorded_at DESC,telemetry_id DESC LIMIT 1) "
        "LEFT JOIN missions m ON m.mission_id=("
        " SELECT mission_id FROM missions WHERE drone_id=d.drone_id "
        " ORDER BY mission_id DESC LIMIT 1) ORDER BY d.drone_id;";
    bool first = true;
    if (sqlite3_prepare_v2(db, droneSql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) out << ",";
            first = false;
            out << "{\"drone_id\":" << sqlite3_column_int(stmt, 0)
                << ",\"name\":\"" << escape(sqlite3_column_text(stmt, 1))
                << "\",\"model\":\"" << escape(sqlite3_column_text(stmt, 2))
                << "\",\"status\":\"" << escape(sqlite3_column_text(stmt, 3))
                << "\",\"battery\":" << sqlite3_column_double(stmt, 4)
                << ",\"altitude\":" << sqlite3_column_double(stmt, 5)
                << ",\"speed\":" << sqlite3_column_double(stmt, 6)
                << ",\"latitude\":" << sqlite3_column_double(stmt, 7)
                << ",\"longitude\":" << sqlite3_column_double(stmt, 8)
                << ",\"mission\":\"" << escape(sqlite3_column_text(stmt, 9))
                << "\",\"mission_status\":\"" << escape(sqlite3_column_text(stmt, 10))
                << "\",\"telemetry_samples\":" << sqlite3_column_int(stmt, 11) << "}";
        }
    }
    if (stmt) sqlite3_finalize(stmt);

    out << "],\"tasks\":[";
    const char* taskSql =
        "SELECT t.task_id,t.task_name,t.priority,t.state,m.mission_name,d.name,"
        "t.cpu_required,t.memory_required,t.power_required,t.created_at,"
        "COALESCE(t.completed_at,'') "
        "FROM tasks t JOIN missions m ON m.mission_id=t.mission_id "
        "JOIN drones d ON d.drone_id=m.drone_id ORDER BY t.task_id DESC;";
    first = true;
    stmt = nullptr;
    if (sqlite3_prepare_v2(db, taskSql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) out << ",";
            first = false;
            out << "{\"task_id\":" << sqlite3_column_int(stmt, 0)
                << ",\"task_name\":\"" << escape(sqlite3_column_text(stmt, 1))
                << "\",\"priority\":\"" << escape(sqlite3_column_text(stmt, 2))
                << "\",\"state\":\"" << escape(sqlite3_column_text(stmt, 3))
                << "\",\"mission\":\"" << escape(sqlite3_column_text(stmt, 4))
                << "\",\"drone\":\"" << escape(sqlite3_column_text(stmt, 5))
                << "\",\"cpu\":" << sqlite3_column_double(stmt, 6)
                << ",\"memory\":" << sqlite3_column_double(stmt, 7)
                << ",\"power\":" << sqlite3_column_double(stmt, 8)
                << ",\"created_at\":\"" << escape(sqlite3_column_text(stmt, 9))
                << "\",\"completed_at\":\"" << escape(sqlite3_column_text(stmt, 10))
                << "\"}";
        }
    }
    if (stmt) sqlite3_finalize(stmt);

    out << "],\"summary\":{\"drone_count\":" << countRows("SELECT COUNT(*) FROM drones")
        << ",\"mission_count\":" << countRows("SELECT COUNT(*) FROM missions")
        << ",\"task_count\":" << countRows("SELECT COUNT(*) FROM tasks")
        << ",\"telemetry_count\":" << countRows("SELECT COUNT(*) FROM telemetry")
        << "}}";
    return out.str();
}

int Database::assignFleetTask(const std::string& droneName, const std::string& taskName)
{
    sqlite3_stmt* stmt = nullptr;
    int drone = -1;
    if (sqlite3_prepare_v2(db, "SELECT drone_id FROM drones WHERE name=?", -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, droneName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) drone = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (drone < 0) return -1;

    const std::string missionName = taskName;
    if (sqlite3_prepare_v2(db,
        "INSERT INTO missions(drone_id,mission_name,status,progress) VALUES(?,?, 'ASSIGNED',0)",
        -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, drone);
    sqlite3_bind_text(stmt, 2, missionName.c_str(), -1, SQLITE_TRANSIENT);
    const bool missionOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!missionOk) return -1;
    const int mission = static_cast<int>(sqlite3_last_insert_rowid(db));

    if (sqlite3_prepare_v2(db,
        "INSERT INTO tasks(mission_id,task_name,priority,state,cpu_required,memory_required,power_required) "
        "VALUES(?,?,'HIGH','ASSIGNED',10,128,4)", -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, mission);
    sqlite3_bind_text(stmt, 2, taskName.c_str(), -1, SQLITE_TRANSIENT);
    const bool taskOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return taskOk ? static_cast<int>(sqlite3_last_insert_rowid(db)) : -1;
}

bool Database::hasTasksForMission(
    int missionId)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "SELECT EXISTS("
        "SELECT 1 FROM tasks "
        "WHERE mission_id=? "
        "LIMIT 1"
        ");";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(
        statement,
        1,
        missionId
    );

    bool exists = false;

    if (sqlite3_step(statement) ==
        SQLITE_ROW)
    {
        exists =
            sqlite3_column_int(
                statement,
                0
            ) != 0;
    }

    sqlite3_finalize(statement);

    return exists;
}

bool Database::beginTransaction()
{
    return execute(
        "BEGIN TRANSACTION;"
    );
}

bool Database::commitTransaction()
{
    return execute(
        "COMMIT;"
    );
}

bool Database::rollbackTransaction()
{
    return execute(
        "ROLLBACK;"
    );
}

int Database::insertTask(
    int missionId,
    const Task& task)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO tasks "
        "(mission_id, task_name, priority, state, "
        "cpu_required, memory_required, "
        "power_required) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(
        statement,
        1,
        missionId
    );

    sqlite3_bind_text(
        statement,
        2,
        task.name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        priorityToString(task.priority),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        4,
        taskStateToString(task.state),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_double(
        statement,
        5,
        task.cpuRequired
    );

    sqlite3_bind_double(
        statement,
        6,
        task.memoryRequired
    );

    sqlite3_bind_double(
        statement,
        7,
        task.powerRequired
    );

    if (sqlite3_step(statement) !=
        SQLITE_DONE)
    {
        sqlite3_finalize(statement);
        return -1;
    }

    const int id =
        static_cast<int>(
            sqlite3_last_insert_rowid(db)
        );

    sqlite3_finalize(statement);

    return id;
}

bool Database::updateTaskState(
    int taskId,
    const std::string& state)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "UPDATE tasks "
        "SET state=?, "
        "completed_at=CASE WHEN ?='COMPLETED' THEN CURRENT_TIMESTAMP "
        "WHEN ?='IN_PROGRESS' THEN NULL ELSE completed_at END "
        "WHERE task_id=?;";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(
        statement,
        1,
        state.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        2,
        state.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        state.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        statement,
        4,
        taskId
    );

    const bool success =
        sqlite3_step(statement) ==
        SQLITE_DONE;

    sqlite3_finalize(statement);
    if (!success) return false;

    sqlite3_stmt* missionStatement = nullptr;
    const char* missionSql =
        "UPDATE missions SET status=?, progress=CASE WHEN ?='COMPLETED' THEN 100.0 "
        "WHEN ?='IN_PROGRESS' THEN 1.0 ELSE progress END "
        "WHERE mission_id=(SELECT mission_id FROM tasks WHERE task_id=?);";
    if (sqlite3_prepare_v2(db, missionSql, -1, &missionStatement, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(missionStatement, 1, state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(missionStatement, 2, state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(missionStatement, 3, state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(missionStatement, 4, taskId);
    const bool missionUpdated = sqlite3_step(missionStatement) == SQLITE_DONE;
    sqlite3_finalize(missionStatement);
    return missionUpdated;
}

bool Database::insertTelemetry(
    int droneId,
    const Telemetry& telemetry)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO telemetry "
        "(drone_id, altitude, speed, battery, "
        "signal, temperature, latitude, longitude) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(
        statement, 1, droneId);

    sqlite3_bind_double(
        statement, 2, telemetry.altitude);

    sqlite3_bind_double(
        statement, 3, telemetry.speed);

    sqlite3_bind_double(
        statement, 4, telemetry.battery);

    sqlite3_bind_double(
        statement, 5, telemetry.signal);

    sqlite3_bind_double(
        statement, 6, telemetry.temperature);

    sqlite3_bind_double(
        statement, 7, telemetry.latitude);

    sqlite3_bind_double(
        statement, 8, telemetry.longitude);

    const bool success =
        sqlite3_step(statement) ==
        SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}

bool Database::insertEvent(
    int droneId,
    const std::string& type,
    const std::string& message,
    const std::string& severity)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO events "
        "(drone_id, event_type, message, severity) "
        "VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(
        statement, 1, droneId);

    sqlite3_bind_text(
        statement, 2, type.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 3, message.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 4, severity.c_str(),
        -1, SQLITE_TRANSIENT);

    const bool success =
        sqlite3_step(statement) ==
        SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}

bool Database::insertFault(
    int droneId,
    const std::string& type,
    const std::string& severity,
    const std::string& description)
{
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO faults "
        "(drone_id, fault_type, severity, description) "
        "VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(
        statement, 1, droneId);

    sqlite3_bind_text(
        statement, 2, type.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 3, severity.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 4, description.c_str(),
        -1, SQLITE_TRANSIENT);

    const bool success =
        sqlite3_step(statement) ==
        SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}

bool Database::resolveFault(int faultId)
{
    std::stringstream sql;

    sql
        << "UPDATE faults "
           "SET resolved=1, "
           "resolved_at=CURRENT_TIMESTAMP "
           "WHERE fault_id="
        << faultId
        << ";";

    return execute(sql.str());
}

void Database::shutdown()
{
    if (db != nullptr)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

Database::~Database()
{
    shutdown();
}

}