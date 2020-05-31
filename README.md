# simple-named-pipe-server
Очень простая, многопоточная серверная и клиентская библиотека Named Pipe, реализованная с использованием C++11.

Проект был проверен на компиляторе *mingw 7.3.0 x64*. Папка *code_blocks* содержит примеры для *IDE Code::Blocks*. Не забудьте в проектах указать свой компилятор, иначе проект не соберется.
 
## Пример сервера

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

## Пример клиента

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
