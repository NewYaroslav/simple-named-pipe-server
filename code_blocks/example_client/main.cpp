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
#include "named-pipe-client.hpp"

using namespace std;

int main() {
    {
        std::cout << "test #1" << std::endl;
        SimpleNamedPipe::NamedPipeClient client("my_server");

        client.on_open = [&]() {
            std::cout << "open, handle: " << client.get_handle() << std::endl;
            client.send("Hello!");
        };
        client.on_message = [&](const std::string &in_message) {
            std::cout << "message " << in_message << ", handle: " << client.get_handle() << std::endl;
            client.send("Yes!");
            //client.stop();
        };
        client.on_close = [&]() {
            std::cout << "close, handle: " << client.get_handle() << std::endl;
        };
        client.on_error = [&](const std::error_code &ec) {
            std::cout << "error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
        };

        client.start();
        std::system("pause");
        client.stop();
    }

    {
        std::cout << "test #2" << std::endl;
        SimpleNamedPipe::NamedPipeClient client("my_server");

        client.on_open = [&]() {
            std::cout << "open, handle: " << client.get_handle() << std::endl;
            client.send("Hey!");
        };
        client.on_message = [&](const std::string &in_message) {
            std::cout << "message " << in_message << ", handle: " << client.get_handle() << std::endl;
            client.close();
        };
        client.on_close = [&]() {
            std::cout << "close, handle: " << client.get_handle() << std::endl;
        };
        client.on_error = [&](const std::error_code &ec) {
            std::cout << "error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
        };
        client.start();
        std::system("pause");
    }

    {
        std::cout << "test #3" << std::endl;
        SimpleNamedPipe::NamedPipeClient client("my_server");

        client.on_open = [&]() {
            std::cout << "open, handle: " << client.get_handle() << std::endl;
            client.send("ping");
        };
        client.on_message = [&](const std::string &in_message) {
            std::cout << "message " << in_message << ", handle: " << client.get_handle() << std::endl;
        };
        client.on_close = [&]() {
            std::cout << "close, handle: " << client.get_handle() << std::endl;
        };
        client.on_error = [&](const std::error_code &ec) {
            std::cout << "error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
        };
        client.start();
        for(int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            std::cout << "send ping" << std::endl;
            client.send("ping");
        }
        std::system("pause");
    }

    {
        std::cout << "test #4" << std::endl;
        SimpleNamedPipe::NamedPipeClient client("my_server");

        client.on_open = [&]() {
            std::cout << "open, handle: " << client.get_handle() << std::endl;
            client.send("Wow!");
        };
        client.on_message = [&](const std::string &in_message) {
            std::cout << "message " << in_message << ", handle: " << client.get_handle() << std::endl;
        };
        client.on_close = [&]() {
            std::cout << "close, handle: " << client.get_handle() << std::endl;
        };
        client.on_error = [&](const std::error_code &ec) {
            std::cout << "error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
        };
        client.start();
        std::system("pause");
    }
    std::cout << "end test" << std::endl;
    std::system("pause");
    return EXIT_SUCCESS;
}
