//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "ChatMessage.hpp"

using boost::asio::ip::tcp;

typedef std::deque<ChatMessage> ChatMessageQueue;

class ChatClient
{
public:
  ChatClient(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
    connect(endpoint_iterator);
  }

  void write(const ChatMessage& msg)
  {
    io_service_.post(
        [this, msg] {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
            write();
        });
  }

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
  void connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator) {
          if (!ec)
            read_header();
        });
  }

  void read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), ChatMessage::HEADER_SIZE),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header())
            read_body();
          else
            socket_.close();
        });
  }

  void read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            read_header();
          } else {
            socket_.close();
          }
        });
  }

  void write()
  {
    boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(),write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              write();
            }
          } else {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  ChatMessage read_msg_;
  ChatMessageQueue write_msgs_;
};

int main_impl(int argc, const char *argv[])
{

    if (argc != 3)
        throw std::invalid_argument("Incorrect number of arguments \n"
                                    "Usage: chat_client <host> <port>");

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
    ChatClient client(io_service, endpoint_iterator);

    std::thread t([&io_service] { io_service.run(); });

    const std::streamsize LINE_SIZE = ChatMessage::MAX_BODY_SIZE + 1;
    std::unique_ptr<char> line(new char[LINE_SIZE]);
    while (std::cin.getline(line.get(), LINE_SIZE)) {
      ChatMessage msg;
      msg.body_length(LINE_SIZE);
      std::memcpy(msg.body(), line.get(), msg.body_length());
      msg.encode_header();
      client.write(msg);
    }

    client.close();
    t.join();
    return EXIT_SUCCESS;
}

int main(int argc, const char *argv[])
{
    try {
        return main_impl(argc, argv);
    } catch (const std::exception &ex) {
        std::cout << "[" << __FUNCTION__ << "]::" << __LINE__ << " Unhandled exception -> " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "[" << __FUNCTION__ << "]::" << __LINE__ << " Unrecognized unhandled exception" << std::endl;
    }

    return EXIT_FAILURE;
}
