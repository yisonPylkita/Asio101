#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "ChatMessage.hpp"

using boost::asio::ip::tcp;

typedef std::deque<ChatMessage> ChatMessageQueue;

class ChatMember
{
public:
  virtual ~ChatMember() {}
  virtual void deliver(const ChatMessage& msg) = 0;
};

typedef std::shared_ptr<ChatMember> ChatMemberPtr;

class ChatRoom
{
public:
  void join(ChatMemberPtr member)
  {
    members_.insert(member);
    for (auto msg: recent_msgs_)
      member->deliver(msg);
  }

  void leave(ChatMemberPtr member)
  {
    members_.erase(member);
  }

  void deliver(const ChatMessage& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto member: members_)
      member->deliver(msg);
  }

private:
  std::set<ChatMemberPtr> members_;
  enum { max_recent_msgs = 100 };
  ChatMessageQueue recent_msgs_;
};

class ChatSession
  : public ChatMember,
    public std::enable_shared_from_this<ChatSession>
{
public:
  ChatSession(tcp::socket socket, ChatRoom& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    read_header();
  }

  void deliver(const ChatMessage& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
      write();
  }

private:
  void read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), ChatMessage::HEADER_SIZE),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header())
            read_body();
          else
            room_.leave(shared_from_this());
        });
  }

  void read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            room_.deliver(read_msg_);
            read_header();
          } else {
            room_.leave(shared_from_this());
          }
        });
  }

  void write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
              write();
          } else {
            room_.leave(shared_from_this());
          }
        });
  }

private:
  tcp::socket socket_;
  ChatRoom& room_;
  ChatMessage read_msg_;
  ChatMessageQueue write_msgs_;
};

class ChatServer
{
public:
  ChatServer(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec) {
          if (!ec)
            std::make_shared<ChatSession>(std::move(socket_), room_)->start();
          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  ChatRoom room_;
};

int main_impl(int argc, const char *argv[])
{
    if (argc != 2)
        throw std::invalid_argument("Incorrect number of arguments \n"
                                    "Usage: chat_server <port>");
    
    boost::asio::io_service io_service;
    tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
    ChatServer server(io_service, endpoint);
    io_service.run();    
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
