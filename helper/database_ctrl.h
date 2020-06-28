/*!
 * \file DatabaseCtrl.h
 * \brief Database controller class and function definitions
 */

#ifndef ZTP_DATABASECTRL_H
#define ZTP_DATABASECTRL_H

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>

class DatabaseCtrl
{
private:
    mongocxx::database m_db;
    mongocxx::client m_client;

public:
    explicit DatabaseCtrl(const char *db_name);

    /// FindOne - Find an entry.
    /// @param coll_name Collection name
    /// @param bson_val Query as BSON object
    /// @return Optional BSON document
    bsoncxx::stdx::optional<bsoncxx::document::value>
    find_one(const char *coll_name, const bsoncxx::document::view_or_value& bson_val) const;

    /// DeleteOne - Delete an entry from the database.
    /// @param coll_name Collection name
    /// @param bson_val Query as BSON object
    /// @return Optional result::delete_result
    bsoncxx::stdx::optional<mongocxx::result::delete_result>
    delete_one(const char *coll_name, const bsoncxx::document::view_or_value& bson_val) const;

    /// InsertOne - Insert an entry to the database.
    /// @param coll_name Collection name
    /// @param bson_val Query as BSON object
    /// @return Optional result::insert_one
    bsoncxx::stdx::optional<mongocxx::result::insert_one>
    insert_one(const char *coll_name, const bsoncxx::document::view_or_value& bson_val) const;

    /// Find - Find entries.
    /// @param coll_name Collection name
    /// @param bson_val Query as BSON object
    /// @param opts Query options
    /// @return mongocxx::cursor
    mongocxx::cursor
    find_many(const char *coll_name, const bsoncxx::document::view_or_value &bson_val,
              const mongocxx::options::find &opts = {}) const;
};


#endif //ZTP_DATABASECTRL_H