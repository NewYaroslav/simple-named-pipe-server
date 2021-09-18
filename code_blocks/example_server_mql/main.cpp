/*
* simple-named-pipe-server - C++ server and client library Named Pipe
*
* Copyright (c) 2020 Elektro Yar. Email: git.electroyar@gmail.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include <iostream>
#include "named-pipe-server.hpp"

using namespace std;

int main() {
    SimpleNamedPipe::NamedPipeServer server("test-pipe");

    server.on_open = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "open, handle: " << connection->get_handle() << std::endl;
    };

    server.on_message = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::string &in_message) {
        /* обрабатываем входящие сообщения */
        //std::cout << "message " << connection->get_handle() << std::endl;
        std::cout << "message " << in_message << ", handle: " << connection->get_handle() << std::endl;
        //connection->send("{\"ping\":1}");
        server.send_all("ping");
    };
    server.on_close = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "close, handle: " << connection->get_handle() << std::endl;
    };
    server.on_error = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::error_code &ec) {
        std::cout << "error, handle: " << connection->get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем сервер */
    server.start();

    /*
    for (size_t i = 0; i < 10000; ++i) {
        std::cout << "send_all " << i << std::endl;
        server.send_all("{\"ping\":1}");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    */

    std::system("pause");
    std::cout << "stop 1" << std::endl;
    server.stop();
    std::cout << "stop 2" << std::endl;
    std::system("pause");
    std::cout << "stop 3" << std::endl;
    server.stop();
    std::cout << "stop 4" << std::endl;
    server.start();
    std::system("pause");
    server.stop();
    std::cout << "close program" << std::endl;
    return EXIT_SUCCESS;
}
