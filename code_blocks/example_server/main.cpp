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
    int num_ping = 0;
    SimpleNamedPipe::NamedPipeServer server("my_server");

    server.on_open = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "open, handle: " << connection->get_handle() << std::endl;
    };

    server.on_message = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::string &in_message) {
        /* обрабатываем входящие сообщения */
        std::cout << "message " << in_message << ", handle: " << connection->get_handle() << std::endl;
        if(in_message.find("Hello!") != std::string::npos) connection->send("Hello to you too! Server order - sleep!");
        else if(in_message.find("Yes!") != std::string::npos) connection->close();
        if(in_message.find("Hey!") != std::string::npos) connection->send("Well hello...");
        else if(in_message.find("ping") != std::string::npos) {
            connection->send("pong");
            ++num_ping;
            if(num_ping > 2) {
                num_ping = 0;
                connection->send("pong");
                connection->send("pong");
            }
        }
    };
    server.on_close = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "close, handle: " << connection->get_handle() << std::endl;
    };
    server.on_error = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::error_code &ec) {
        std::cout << "error, handle: " << connection->get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем сервер */
    server.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::system("pause");

    server.send_all("ping+");
    std::system("pause");

    std::cout << "stop" << std::endl;
    server.stop();

    /* для провери повторной остановки сервера */
    std::cout << "stop-2" << std::endl;
    server.stop();

    std::cout << "close program" << std::endl;
    return EXIT_SUCCESS;
}
