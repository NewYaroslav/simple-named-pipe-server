# simple-named-pipe-server
Очень простая, многопоточная серверная и клиентская библиотека Named Pipe, реализованная с использованием C++11.

Проект был проверен на компиляторе *mingw 7.3.0 x64*. Папка *code_blocks* содержит примеры для *IDE Code::Blocks*. Не забудьте в проектах указать свой компилятор, иначе проект не соберется.
 
## Особенности использования Named Pipe сервера 

* Для отправки сообщения всем клиентам используйте метод 'send_all'.
* Чтобы отправить сообщение конкретному клиенту, используйте метод 'send' клиента, указатель на которого передается в функции обратного вызова в момент наступления события 'on_open' или 'on_message'.
* Чтобы узнать количество подключений, используйте  метод 'get_connections()'.

## Важные фиксы

* Методы 'get_connections()' и 'send_all' можно вызывать внутри 'on_open', 'on_message', 'on_close', 'on_error'

## Пример сервера на C++

```cpp
#include <iostream>
#include "named-pipe-server.hpp"

int main() {
    /* в конструкторе сервера можно также задать размер буфера */
    SimpleNamedPipe::NamedPipeServer server("my_server");

    /* обработчики событий */
    server.on_open = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "open, handle: " << connection->get_handle() << std::endl;
    };

    server.on_message = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, 
			const std::string &in_message) {
        /* обрабатываем входящие сообщения */
        std::cout << "message " << in_message << ", handle: " << connection->get_handle() << std::endl;
        connection->send("ok");
    };

    server.on_close = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection) {
        std::cout << "close, handle: " << connection->get_handle() << std::endl;
    };

    server.on_error = [&](SimpleNamedPipe::NamedPipeServer::Connection* connection, const std::error_code &ec) {
        std::cout << "error, handle: " << connection->get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем сервер */
    server.start();
    std::system("pause");

    /* останавливаем сервер 
     * (деструктор класса сам выполнит остановку, вызывать не обязательно)
     */
    server.stop();
    std::cout << "close program" << std::endl;
    return EXIT_SUCCESS;
}
```

## Пример клиента на C++

```cpp
#include <iostream>
#include "named-pipe-client.hpp"

using namespace std;

int main() {
    /* в конструкторе клиента можно также задать размер буфера */
    SimpleNamedPipe::NamedPipeClient client("my_server");

    /* обработчики событий */
    client.on_open = [&]() {
	std::cout << "open, handle: " << client.get_handle() << std::endl;
	client.send("Hello!");
    };

    client.on_message = [&](const std::string &in_message) {
	std::cout << "message " << in_message << ", handle: " << client.get_handle() << std::endl;
	client.send("ok");
	//client.close(); // можно закрыть соединение
    };
	
    client.on_close = [&]() {
	std::cout << "close, handle: " << client.get_handle() << std::endl;
    };
	
    client.on_error = [&](const std::error_code &ec) {
	std::cout << "error, handle: " << client.get_handle() << ", what " << ec.value() << std::endl;
    };

    /* запускаем клиент */
    client.start();
	
    std::system("pause");
    std::cout << "close program" << std::endl;
    return EXIT_SUCCESS;
}
```

## Пример клиента для Meta Trader 5

```
//+------------------------------------------------------------------+
//|                                            named_pipe_client.mq5 |
//|                                     Copyright 2021, NewYaroslav. |
//|          https://github.com/NewYaroslav/simple-named-pipe-server |
//+------------------------------------------------------------------+
#include "..\include\named_pipe_client.mqh"

input string pipe_name      = "test-pipe"; // Named Pipe for the stream of quotes
input int    timer_period   = 10; // Timer period for processing incoming messages

NamedPipeClient     *pipe           = NULL;

//+------------------------------------------------------------------+
//| NamedPipeClient function                                   |
//+------------------------------------------------------------------+
void NamedPipeClient::on_open(NamedPipeClient *pointer) {
    Print("open connection with ", pointer.get_pipe_name());
}

void NamedPipeClient::on_close(NamedPipeClient *pointer) {
    Print("closed connection with ", pointer.get_pipe_name());
}

void NamedPipeClient::on_message(NamedPipeClient *pointer, const string &message) {
    Print("message: " + message);
}

void NamedPipeClient::on_error(NamedPipeClient *pointer, const string &error_message) {
    Print("Error! What: " + error_message);
}

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit() {
    if (pipe == NULL) {
        if ((pipe = new NamedPipeClient(pipe_name)) == NULL) return(false);
    }

    EventSetMillisecondTimer(timer_period);
    return(INIT_SUCCEEDED);
}
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason) {
    EventKillTimer();
    if (pipe != NULL) delete pipe;
}
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick() {

}
//+------------------------------------------------------------------+
//| Timer function                                                   |
//+------------------------------------------------------------------+
void OnTimer() {
    static int ticks = 0;
    if (pipe != NULL) pipe.on_timer();
    
    ticks += timer_period;
    if (ticks >= 1000) {
        ticks = 0;

        if (pipe != NULL) {
            if (pipe.connected()) {
                const string str_ping = "ping";
                pipe.write(str_ping);
            } // if (pipe.connected())
        } // if (pipe != NULL)
    } // if (ticks >= 1000)
}
//+------------------------------------------------------------------+
```
