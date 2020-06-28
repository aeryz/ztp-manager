//
// Created by aeryz on 12/3/19.
//

// TODO: Deadline should be added.

#include "server.h"
#include "manager.h"
#include "../helper/data_types.h"
#include <boost/thread/thread.hpp> // For async server

#include "../external/rapidjson/document.h"     // For creating JSON document
#include "../external/rapidjson/writer.h"       // For JSON to string
#include "../external/rapidjson/stringbuffer.h" // For JSON to string

#include "../external/easylogging++/easylogging++.h"

#define PORT 13

// TCP_SERVER
TCPServer::TCPServer(boost::asio::io_context& io_context)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), PORT))
{
    start_accept();
}


void TCPServer::start_accept()
{
    TCPConnection::tcp_ptr new_connection =
            TCPConnection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
                           boost::bind(&TCPServer::handle_accept, this, new_connection,
                                       boost::asio::placeholders::error));
}

void TCPServer::handle_accept(const TCPConnection::tcp_ptr& new_connection,
                              const boost::system::error_code& error)
{
    if (!error)
    {
        new_connection->start();
    }

    start_accept();
}
// TCP_SERVER END

// TCP CONNECTION
TCPConnection::tcp_ptr TCPConnection:: create(boost::asio::io_context& io_context)
{
    return tcp_ptr(new TCPConnection(io_context));
}

tcp::socket& TCPConnection::socket()
{
    return socket_;
}

void TCPConnection::start()
{
    // Read until "\r\n" is seen
    boost::asio::async_read_until(socket_, m_read_buf_, "\r\n",
                            boost::bind(&TCPConnection::handle_read, shared_from_this(),
                                        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void TCPConnection::handle_write(const boost::system::error_code& /*error*/,
                                 size_t n_bytes)
{
    LOG(INFO) << n_bytes << " transferred";
}

void TCPConnection::handle_read(const boost::system::error_code& err,
                                size_t n_bytes)
{
    if (err)
    {
        // Ignore eof errors
        if (err.value() == 104 || err.value() == 2)
            return;
        throw std::runtime_error(err.message());
    }

    ServerResponseData res;

    if (n_bytes > 0)
    {
        m_read_buf_.commit(n_bytes);
        auto buffers = m_read_buf_.data();
        std::string data = std::string(boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));
        LOG(DEBUG) << "Read: " << data;
        res = Manager::exec_cmd(std::move(data));
    }
    else
    {
        LOG(ERROR) << "number of bytes read is <= 0";
        res = ServerResponseData{INTERNAL, "Number of bytes read is <= 0"};
    }

    // TODO: Maybe error handling here
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    // If response data is convertible to JSON, convert it otherwise send it as string.
    rapidjson::Document in_document;
    if (in_document.Parse(res.data.c_str()).HasParseError())
        document.AddMember("data", rapidjson::StringRef(res.data.c_str()), allocator);
    else
        document.AddMember("data", in_document, allocator);

    document.AddMember("status", res.err_code, allocator);

    rapidjson::StringBuffer buffer;
    buffer.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    m_write_buf_ = buffer.GetString();

    boost::asio::async_write(socket_, boost::asio::buffer(m_write_buf_),
                             boost::asio::transfer_all(),
                             boost::bind(&TCPConnection::handle_write, shared_from_this(),
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
}


// TCP CONNECTION END