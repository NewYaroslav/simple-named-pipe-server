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
#include "named-pipe-client.hpp"

using namespace std;

int main() {
    int num_ping = 0;
    SimpleNamedPipe::NamedPipeServer server("my_server");

    server.on_open = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "server open, handle: " << connection->get_handle() << std::endl;
    };

    server.on_message = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::string &in_message) {
        /* обрабатываем входящие сообщения */
        std::cout << "server message " << in_message << ", handle: " << connection->get_handle() << std::endl;
        if(in_message.find("Hello!") != std::string::npos) connection->send("Hello to you too! Server order - sleep!");

        //if(in_message.find("Yes!") != std::string::npos) connection->close();

        if(in_message.find("Hey!") != std::string::npos) connection->send("Well hello...");

        if(in_message.find("ping") != std::string::npos) {
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
        std::cout << "server close, handle: " << connection->get_handle() << std::endl;
    };
    server.on_error = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::error_code &ec) {
        std::cout << "server error, handle: " << connection->get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем сервер */
    server.start();

    /* создаем клиента */
    SimpleNamedPipe::NamedPipeClient client("my_server");

    client.on_open = [&]() {
        std::cout << "client open, handle: " << client.get_handle() << std::endl;
        client.send("Hello!");
    };

    client.on_message = [&](const std::string &in_message) {
        std::cout << "client message " << in_message << ", handle: " << client.get_handle() << std::endl;
        client.send("Yes!");
        //client.stop();
    };
    client.on_close = [&]() {
        std::cout << "client close, handle: " << client.get_handle() << std::endl;
    };

    client.on_error = [&](const std::error_code &ec) {
        std::cout << "client error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем клиента */
    client.start();

    /* тесты */
    std::cout << "#1" << std::endl;
    std::system("pause");
    server.send_all("ping+");
    std::cout << "#2" << std::endl;
    std::system("pause");
    std::cout << "#3" << std::endl;
    std::cout << "server stop" << std::endl;
    server.stop();
    std::cout << "server stop 2" << std::endl;
    server.stop();
    std::cout << "#4" << std::endl;
    std::system("pause");
    server.start();
    std::cout << "#5" << std::endl;
    std::system("pause");
    std::cout << "close program" << std::endl;
    return EXIT_SUCCESS;
}
