//
// Created by aeryz on 12/3/19.
//

#ifndef MANAGER_SERVER_H
#define MANAGER_SERVER_H

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class TCPConnection
        : public boost::enable_shared_from_this<TCPConnection>
{
public:
    typedef boost::shared_ptr<TCPConnection> tcp_ptr;

    static tcp_ptr create(boost::asio::io_context& io_context);

    tcp::socket& socket();

    void start();

private:
    explicit TCPConnection(boost::asio::io_context& io_context)
            : socket_(io_context) {}

    void handle_write(const boost::system::error_code& /*error*/,
                      size_t /*bytes_transferred*/);

    void handle_read(const boost::system::error_code& err,
                     size_t n_bytes);

    tcp::socket socket_;
    boost::asio::streambuf m_read_buf_;
    std::string m_write_buf_;
};

class TCPServer
{
public:
    explicit TCPServer(boost::asio::io_context& io_context);

private:
    void start_accept();

    void handle_accept(const TCPConnection::tcp_ptr& new_connection,
                       const boost::system::error_code& error);

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};

#endif //MANAGER_SERVER_H
