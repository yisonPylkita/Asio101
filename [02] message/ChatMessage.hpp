//
// ChatMessage.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>

class ChatMessage
{
public:
  enum { HEADER_SIZE = 4 };
  enum { MAX_BODY_SIZE = 512 };

  ChatMessage()
    : body_length_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return HEADER_SIZE + body_length_;
  }

  const char* body() const
  {
    return data_ + HEADER_SIZE;
  }

  char* body()
  {
    return data_ + HEADER_SIZE;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > MAX_BODY_SIZE)
      body_length_ = MAX_BODY_SIZE;
  }

  bool decode_header()
  {
    char header[HEADER_SIZE + 1] = "";
    std::strncat(header, data_, HEADER_SIZE);
    body_length_ = std::atoi(header);
    if (body_length_ > MAX_BODY_SIZE)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[HEADER_SIZE + 1] = "";
    std::sprintf(header, "%4d", static_cast<int>(body_length_));
    std::memcpy(data_, header, HEADER_SIZE);
  }

private:
  char data_[HEADER_SIZE + MAX_BODY_SIZE];
  std::size_t body_length_;
};
