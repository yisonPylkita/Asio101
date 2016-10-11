//
// ChatServer.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>

int main_impl(int argc, const char *argv[])
{    
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
