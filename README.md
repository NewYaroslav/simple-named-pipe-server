# simple-named-pipe-server
Очень простая, многопоточная серверная и клиентская библиотека Named Pipe, реализованная с использованием C++11.

Проект был проверен на компиляторе *mingw 7.3.0 x64*. Папка *code_blocks* содержит примеры для *IDE Code::Blocks*. Не забудьте в проектах указать свой компилятор, иначе проект не соберется.
 
## Особенности использования Named Pipe сервера 

* Для отправки сообщения всем клиентам используйте метод 'send_all'.
* Чтобы отправить сообщение конкретному клиенту, используйте метод 'send' клиента, указатель на которого передается в функции обратного вызова в момент наступления события 'on_open' или 'on_message'.
* Чтобы узнать количество подключений, используйте  метод 'get_connections()'.
* Методы 'get_connections()' и 'send_all' НЕЛЬЗЯ ВЫЗЫВАТЬ ВНУТРИ 'on_open', 'on_message', 'on_close', 'on_error'! Это приведет к блокировке мьютекса. Чтобы избежать блокировки, вы можете создать отдельный поток внутри функции обратного вызова, внутри которого и вызвать нужный вам метод.

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

## Пример клиента для Meta Trader 5

```
#include "named_pipe_client.mqh"
#include <Trade\SymbolInfo.mqh>

input string   pipe_name = "mega-connector-quotes"; // Именованный канал для потока котировок

CSymbolInfo      *m_symbol;         // pointer to the object-symbol
NamedPipeClient pipe;

bool     is_prev_connected = false;
bool     is_connected      = false;
string   symbol;
string   symbol_digits_str;
string   server_name;
int      symbol_digits     = 0; 

int OnInit() {
   is_connected = pipe.open(pipe_name);
   symbol = Symbol();
   symbol_digits = Digits();
   symbol_digits_str = IntegerToString(symbol_digits);
   server_name = AccountInfoString(ACCOUNT_SERVER);
   
   if(m_symbol == NULL) {
      if((m_symbol = new CSymbolInfo) == NULL) return(false);
   }
   if(!m_symbol.Name(symbol)) return(false);
   return(INIT_SUCCEEDED);
}
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason) {
   if(is_connected) pipe.close();
   if(m_symbol!=NULL) delete m_symbol;
   is_connected = false;
}
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick() {
   if(is_connected) {
      if (!is_prev_connected) {
         is_prev_connected = true;
         Print("Pipe " + pipe_name + " connected");
      }
   } else {
      if (is_prev_connected) {
         is_prev_connected = false;
         Print("Pipe " + pipe_name + " disconnected");
      }
   }
   if(is_connected) {
      if(!m_symbol.RefreshRates()) {
         if(pipe.get_bytes_read() > 0) {
            pipe.read();
         }
         return;
      }

      const double t = (double)TimeGMT();
      string json_body;
      StringConcatenate(json_body,json_body,"{\"e\":0,\"v\":0,\"d\":{\"t\":",DoubleToString(t));
      StringConcatenate(json_body,json_body,",\"s\":\"",symbol);
      StringConcatenate(json_body,json_body,"\",\"b\":",DoubleToString(m_symbol.Bid(),symbol_digits));
      StringConcatenate(json_body,json_body,",\"a\":",DoubleToString(m_symbol.Ask(),symbol_digits));
      StringConcatenate(json_body,json_body,",\"sn\":\"",server_name);
      StringConcatenate(json_body,json_body,"\",\"d\":",symbol_digits_str,"}}");

      if(!pipe.write(json_body)) {
         pipe.close();
         is_connected = false;
      }
      if(pipe.get_bytes_read() > 0) {
		 Print("read " + pipe.read());
      }
   } else {
      is_connected = pipe.open(pipe_name);
   }
}
```
