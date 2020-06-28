//
// Created by aeryz on 11/20/19.
//

// Internal
#include "manager.h"
#include "../helper/data_types.h"

// Std
#include <iostream>
#include <regex>
#include <bsoncxx/builder/stream/document.hpp>
#include <unistd.h>
#include <sys/wait.h>

// External local
#include "../external/rapidjson/document.h"
#include "../external/rapidjson/stringbuffer.h"
#include "../external/rapidjson/writer.h"
#include "../external/easylogging++/easylogging++.h"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

ServerResponseData Manager::scan_new(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    if (!data.HasMember("ssh-username") || !data.HasMember("ssh-password") || !data.HasMember("targets"))
        return ServerResponseData{BAD_REQUEST, "Some fields are missing in the JSON"};

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);

    // Give json data as an argument to ztp
    char *args[3] = {const_cast<char *>("ztp"), const_cast<char *>(buffer.GetString()), nullptr};

    int fd[2];
    if (pipe(fd))
        return ServerResponseData{INTERNAL, "Pipe error."};

    int pid = fork();
    if (pid < 0)
        return ServerResponseData{INTERNAL, "Couldn't start a child process to start ztp."};
    if (pid == 0)
    {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        // TODO: This path should be dynamic
        execve("/usr/local/bin/ztp", args, environ);
        exit(1);
    }
    close(fd[1]);

    FILE *fp = fdopen(fd[0], "r");
    if (!fp)
        return ServerResponseData{INTERNAL, "Failed to read from 'ztp'. Scan might be started or not."};

    std::string dynamic_buf;
    std::array<char, 128> buf{};
    while (fgets(buf.data(), 128, fp))
        dynamic_buf += buf.data();

    // TODO: This wait thing can be handled with usleep&WNOHANG combination (would be more convenient)
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) == 0)
        return ServerResponseData{UNKNOWN, "Exit code: " + std::to_string(WEXITSTATUS(status))};

    return ServerResponseData(SUCCESS, dynamic_buf);
}

ServerResponseData Manager::scan_stop(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    if (!data.HasMember("id"))
        return ServerResponseData{BAD_REQUEST, "'id' field is needed"};

    const auto result = db.find_one("scan", document{} << "_id" << bsoncxx::oid(data["id"].GetString()) << finalize);
    if (!result)
        return ServerResponseData{NOT_FOUND, "Couldn't find the scan entry"};

    int status = (*result).view()["status"].get_int32().value;
    if (status != ScanStatus::ONGOING)
        return ServerResponseData{BAD_REQUEST, "Scan's status is not ongoing"};

    int pid = (*result).view()["pid"].get_int32().value;
    if (pid <= 0)
        return ServerResponseData{INTERNAL, "'pid' is <= 0"};

    if (kill(pid, SIGTERM))
        return ServerResponseData{INTERNAL, "Couldn't stop the scan"};

    return ServerResponseData();
}

ServerResponseData Manager::scan_status(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    if (!data.HasMember("id"))
        return ServerResponseData{BAD_REQUEST, "'id' field is needed"};
    const auto result = db.find_one("scan", document{} << "_id" << bsoncxx::oid(data["id"].GetString()) << finalize);
    if (!result)
        return ServerResponseData{NOT_FOUND, "Couldn't find the scan entry"};

    return ServerResponseData{SUCCESS, bsoncxx::to_json((*result).view())};
}

ServerResponseData Manager::scan_delete(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    if (!data.HasMember("id"))
        return ServerResponseData{BAD_REQUEST, "'id' field is needed"};
    const auto result = db.find_one("scan", document{} << "_id" << bsoncxx::oid(data["id"].GetString()) << finalize);
    if (!result)
        return ServerResponseData{NOT_FOUND, "No entry found."};

    return ServerResponseData();
}

ServerResponseData Manager::scan_list(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    int64_t limit = 3;
    if (data.HasMember("limit"))
        limit = data["limit"].GetInt();

    // Ascending order
    auto order = document{} << "$natural" << -1 << finalize;
    auto opts = mongocxx::options::find{};
    opts.sort(order.view());
    opts.limit(limit);

    auto res_cursor = db.find_many("scan", document{} << finalize, opts);

    bsoncxx::builder::stream::document doc_builder{};
    auto doc = doc_builder << "scans" << bsoncxx::builder::stream::open_array;
    for (auto &&elem: res_cursor)
        doc = doc << elem;

    auto final = doc << bsoncxx::builder::stream::close_array << finalize;

    return ServerResponseData{SUCCESS, bsoncxx::to_json(final)};
}

ServerResponseData Manager::report_get(const DatabaseCtrl &db, const rapidjson::Document &data)
{
    if (!data.HasMember("id"))
        return ServerResponseData{BAD_REQUEST, "'id' field is needed"};

    const auto scan_result = db.find_one("scan",
                                         document{} << "_id" << bsoncxx::oid(data["id"].GetString()) << finalize);
    if (!scan_result)
        return ServerResponseData{NOT_FOUND, "No entry found."};

    bsoncxx::builder::stream::document doc_builder{};
    auto doc = doc_builder << "creation_date" << (*scan_result).view()["creation_date"].get_date()
                           << "end_date"      << (*scan_result).view()["end_date"].get_date()
                           << "status"        << (*scan_result).view()["status"].get_int32()
                           << "targets"       << bsoncxx::builder::stream::open_array;

    auto target_cursor = db.find_many("target",
                                      document{} << "scan_hash" << (*scan_result).view()["_id"].get_oid() << finalize);
    for (auto &&target: target_cursor)
    {
        bsoncxx::builder::stream::document target_builder{};
        auto target_doc = target_builder << "ip"      << target["ip"].get_utf8()
                                         << "os"      << target["os"].get_utf8()
                                         << "reports" << bsoncxx::builder::stream::open_array;

        auto report_cursor = db.find_many("dynamicreport",
                                          document{} << "target" << target["_id"].get_oid() << finalize);

        for (auto &&report: report_cursor)
        {
            const auto rep_opt = db.find_one("staticreport",
                                             document{} << "_id" << report["static_report"].get_utf8().value
                                                        << finalize);

            auto open_state = target_doc << bsoncxx::builder::stream::open_document;
            if (!rep_opt) {
                LOG(WARNING) << "Report not found: "
                             << report["static_report"].get_utf8().value;
                open_state = open_state << "static"  << ""
                                        << "dynamic" << report["dynamic_report"].get_utf8();
            }
            else {
                open_state = open_state << "static"  << (*rep_opt).view()
                                        << "dynamic" << report["dynamic_report"].get_utf8();
            }

            target_doc = open_state << bsoncxx::builder::stream::close_document;
        }

        const auto target_final = target_doc << bsoncxx::builder::stream::close_array << finalize;
        doc = doc << target_final;
    }

    const auto final_doc = doc << bsoncxx::builder::stream::close_array << finalize;
    return ServerResponseData{SUCCESS, bsoncxx::to_json(final_doc)};
}

ServerResponseData Manager::exec_cmd(std::string cmd)
{
    // Remove unprintable characters
    cmd.erase(
            std::remove_if(cmd.begin(), cmd.end(), [](char c){ return !isprint(c); }),
            cmd.end()
    );

    // CATEGORY ACTION "JSON DATA"
    std::regex r(R"( *([a-z]+) +([a-z]+) +\"(.*?)\" *)");
    std::smatch match;

    // No match means that request is bad
    if (!std::regex_match(cmd, match, r) && match.size() == 4)
        return ServerResponseData{BAD_REQUEST, "Bad request."};

    rapidjson::Document d;
    if (d.Parse(match[3].str().c_str()).HasParseError())
        return ServerResponseData{BAD_REQUEST, "Data is not in correct JSON format"};

    const DatabaseCtrl db(ZTP_DB);

    try {
        if (match[1] == "scan")
        {
            if (match[2] == "new")
                return scan_new(db, d);
            else if (match[2] == "list")
                return scan_list(db, d);
            else if (match[2] == "status")
                return scan_status(db, d);
            else if (match[2] == "delete")
                return scan_delete(db, d);
            else if (match[2] == "stop")
                return scan_stop(db, d);
        }
        else if (match[1] == "report")
        {
            if (match[2] == "get")
                return report_get(db, d);
        }
    }
    catch (std::exception &e)
    {
        // This handles all the possible internal database errors.
        LOG(ERROR) << e.what();
        return ServerResponseData{INTERNAL, e.what()};
    }

    return ServerResponseData{BAD_REQUEST, "Bad request."};
}