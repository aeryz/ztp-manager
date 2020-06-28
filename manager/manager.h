/*!
 * \file manager.h
 * \brief Definitions of program's operations.
 */

#ifndef ZTP_MANAGER_H
#define ZTP_MANAGER_H

#include "../helper/database_ctrl.h"
#include "../helper/data_types.h"
#include "../external/rapidjson/document.h"

/*!
 * \namespace Manager
 */
namespace Manager {
    /// scan_status - Get the scan from db and prints its status
    /// @param db DatabaseCtrl object
    /// @param data Parsed JSON data (id is required)
    /// @return ServerResponseData
    ServerResponseData scan_status(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// scan_delete - Delete the scan from db
    /// @param db DatabaseCtrl object
    /// @param data Parsed JSON data (id is required)
    /// @return ServerResponseData
    ServerResponseData scan_delete(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// scan_new - Start a new scan
    /// @param db DatabaseCtrl object
    /// @param data Parsed JSON data (targets, ssh-username, ssh-password are required)
    /// @return ServerResponseData
    ServerResponseData scan_new(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// scan_stop - Stop a scan
    /// @param db DatabaseCtrl object
    /// @param data Parsed JSON data (id is required)
    /// @return ServerResponseData
    ServerResponseData scan_stop(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// scan_list - List scans
    /// @param db DatabaseCtrl object
    /// @param data Parsed JSON data (limit is required)
    /// @return ServerResponseData
    ServerResponseData scan_list(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// report_get - Print a scan's report
    /// @param db DatabaseCtrl object
    /// @param vm Parsed program options (id is required)
    /// @return ServerResponseData
    ServerResponseData report_get(const DatabaseCtrl &db, const rapidjson::Document &data);

    /// exec_cmd - Parse and execute a command
    /// @param cmd String command which read from the connection
    /// @return ServerResponseData
    ServerResponseData exec_cmd(std::string cmd);
}

#endif //ZTP_MANAGER_H
