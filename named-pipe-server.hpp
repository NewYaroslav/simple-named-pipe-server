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
#ifndef NAMED_PIPE_SERVER_HPP_INCLUDED
#define NAMED_PIPE_SERVER_HPP_INCLUDED

#include <iostream>
#include <windows.h>
#include <mutex>
#include <atomic>
#include <future>
#include <system_error>
#include <list>
#include <vector>

namespace SimpleNamedPipe {

    /** \brief Класс сервера именованных каналов
     */
    class NamedPipeServer {
    private:
        HANDLE pipe = INVALID_HANDLE_VALUE;
        std::future<void> named_pipe_future;    /**< Поток обработки новых подключений */
        std::atomic<bool> is_reset;             /**< Команда завершения работы */
        std::atomic<bool> is_error;             /**< Ошибка сервера */

        /* переменные для разблокировки ConnectNamedPipe */
        std::string connection_pipe_name;
        std::atomic<bool> is_connection;
        std::mutex connection_pipe_name_mutex;

        /** \brief Класс настроек соединения
         */
        class Config {
        public:
            std::string name;   /**< Имя именованного канала */
            size_t buffer_size; /**< Размер буфера для чтения и записи */
            size_t timeout;     /**< Время ожидания */

            Config() : name("server"), buffer_size(2048), timeout(50) {
            };
        } config;   /**< Настройки сервера */

    public:

        /** \brief Класс соединения
         */
        class Connection {
        private:
            HANDLE pipe = INVALID_HANDLE_VALUE;     /**< хендлер именованного канала */
            std::future<void> connection_future;    /**< Поток обработки входящих сообщений */
            std::atomic<bool> is_reset;             /**< Команда завершения работы */
            std::atomic<bool> is_error;             /**< Состояние ошибки */
            std::atomic<bool> is_close;             /**< Флаг закрытия соединения */

            std::function<void(Connection*)> &on_open;
            std::function<void(Connection*, const std::string &in_message)> &on_message;
            std::function<void(Connection*)> &on_close;
            std::function<void(Connection*, const std::error_code &)> &on_error;

            size_t buffer_size = 1024;              /**< Размер буфера */

            /** \brief Прочитать сообщение
             */
            void read_message() noexcept {
                if(is_error) {
                    std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    return;
                }
                /* проверяем наличие данных в кнале */
                DWORD bytes_to_read = 0;
                BOOL success = PeekNamedPipe(pipe, NULL, 0, NULL, &bytes_to_read, NULL);
                DWORD err = GetLastError();
                if(!success) {
                    /* если соединение закрыто, вернется ERROR_PIPE_NOT_CONNECTED */
                    if(err == ERROR_PIPE_NOT_CONNECTED) {
                        is_error = true;
                        std::this_thread::yield();
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                        return;
                    } else
                    if(err == ERROR_BROKEN_PIPE) {
                        is_error = true;
                        std::this_thread::yield();
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                        return;
                    }
                }
                if(bytes_to_read == 0) {
                    std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    return;
                }
                std::vector<char> buf(buffer_size);
                DWORD bytes_read = 0;

                success = ReadFile(
                    pipe,
                    &buf[0],
                    buffer_size,
                    &bytes_read,
                    NULL);
                err = GetLastError();
                if(!success || bytes_read == 0) {
                    if(err == ERROR_BROKEN_PIPE) {
                        is_error = true;
                        std::this_thread::yield();
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                        return;
                    } else {
                        on_error(this,std::error_code(static_cast<int>(GetLastError()), std::generic_category()));
                    }
                    is_error = true;
                }
                on_message(this,std::string(buf.begin(),buf.begin() + bytes_read));
            }

        public:

            Connection(
                const HANDLE _pipe,
                std::function<void(Connection*)> &_on_open,
                std::function<void(Connection*, const std::string &in_message)> &_on_message,
                std::function<void(Connection*)> &_on_close,
                std::function<void(Connection*, const std::error_code &)> &_on_error,
                const size_t _buffer_size) :
                    pipe(_pipe),
                    on_open(_on_open),
                    on_message(_on_message),
                    on_close(_on_close),
                    on_error(_on_error),
                    buffer_size(_buffer_size) {
                is_reset = false;
                is_error = false;
                is_close = false;
                connection_future = std::async(std::launch::async,[&]() {
                    on_open(this);
                    while(!is_reset && !is_error) {
                        read_message();
                    }
                    /* очищаем буфер только когда соединение было закрыто не сбросом */
                    if(pipe != INVALID_HANDLE_VALUE) {
                        if(!is_reset) FlushFileBuffers(pipe);
                        DisconnectNamedPipe(pipe);
                        CloseHandle(pipe);
                    }
                    on_close(this);
                    is_close = true;
                });
            }

            ~Connection() {
                is_reset = true;
                CancelIo(pipe);
                if(connection_future.valid()) {
                    try {
                        connection_future.wait();
                        connection_future.get();
                    }
                    catch(...) {}
                }
            }

            /** \brief Отправить сообщение
             * \param out_message Сообщение
             * \param callback Обратный вызов для ошибки
             */
            void send(
                    const std::string &out_message,
                    const std::function<void(const std::error_code &ec)> &callback = nullptr) noexcept {
                if(is_error) {
                    if(callback != nullptr) {
                        callback(std::error_code(static_cast<int>(GetLastError()), std::generic_category()));
                    }
                    return;
                }
                DWORD bytes_written = 0;
                BOOL success = WriteFile(
                    pipe,
                    out_message.c_str(),    // буфер для записи
                    out_message.size(),     // количество байтов для записи
                    &bytes_written,         // количество записанных байтов
                    NULL);                  // не перекрывается I/O

                if(!success || out_message.size() != bytes_written) {
                    /* ошибка записи, закрываем соединение */
                    if(callback != nullptr) {
                        callback(std::error_code(static_cast<int>(GetLastError()), std::generic_category()));
                    }
                    on_error(this,std::error_code(static_cast<int>(GetLastError()), std::generic_category()));
                    CancelIo(pipe);
                    is_error = true;
                }
            }

            /** \brief Закрыть соединение
             */
            inline void close() noexcept {
                is_reset = true;
                CancelIo(pipe);
            }

            /** \brief Проверить закрытие соединения
             * \return Вернет true, если соединение закрыто
             */
            inline bool check_close() noexcept {
                return is_close;
            }

            inline HANDLE get_handle() noexcept {
                return pipe;
            }
        };

        inline void clear_connections() noexcept {
            /* удаляем потоки, где соединение закрыто */
            std::lock_guard<std::recursive_mutex> lock(connections_mutex);
            if(connections.size() == 0) return;
            auto it = connections.begin();
            while(it != connections.end()) {
                if(it->get()->check_close()) {
                    it = connections.erase(it);
                    continue;
                }
                it++;
            }
        }

        inline void reset_connections() noexcept {
            /* удаляем потоки, где соединение закрыто */
            std::lock_guard<std::recursive_mutex> lock(connections_mutex);
            if(connections.size() == 0) return;
            connections.clear();
        }

    private:

        std::list<std::shared_ptr<Connection>> connections; /**< Список соединений */
        std::recursive_mutex connections_mutex;

        /** \brief Инициализировать сервер
         *
         * \param config Настройки сервера
         * \return Вернет true, если инициализация прошла успешно
         */
        bool init(Config &config) noexcept {
            if(named_pipe_future.valid()) return false;
            std::string pipename("\\\\.\\pipe\\");
            if(config.name.find("\\") != std::string::npos) return false;
            pipename += config.name;
            if(pipename.length() > 256) return false;

            /* запоминаем имя соединения */
            {
                std::lock_guard<std::mutex> lock(connection_pipe_name_mutex);
                connection_pipe_name = pipename;
            }

            named_pipe_future = std::async(std::launch::async,[
                    &,
                    pipename,
                    config]() {
                while(!is_reset) {
                    pipe = CreateNamedPipeA(
                      (LPCSTR)pipename.c_str(), // имя канала
                      PIPE_ACCESS_DUPLEX |      // двунаправленный доступ
                      FILE_FLAG_OVERLAPPED,
                      PIPE_TYPE_MESSAGE |       // message type pipe
                      PIPE_READMODE_MESSAGE |   // message-read mode
                      PIPE_WAIT,                // blocking mode
                      PIPE_UNLIMITED_INSTANCES, // max. instances
                      config.buffer_size,       // output buffer size
                      config.buffer_size,       // input buffer size
                      config.timeout,           // client time-out
                      NULL);                    // default security attribute

                    if(pipe == INVALID_HANDLE_VALUE) {
                        std::cerr << "NamedPipeServer::init(), CreateNamedPipeA failed, GLE=" << GetLastError() << std::endl;
                        is_error = false;
                        /* удаляем потоки, где соединение закрыто */
                        //clear_connections();
                        reset_connections();
                        std::this_thread::yield();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        return;
                    }

                    /* устанавливем флаг соединения (необходим для хака во время остановки сервера) */
                    is_connection = true;

                    /* ждем соединения с сервером */
                    BOOL named_pipe_connected;
                    named_pipe_connected = ConnectNamedPipe(pipe, NULL) ?
                        TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

                    /* снимаем флаг соединения (необходим для хака во время остановки сервера) */
                    is_connection = false;

                    /* если бы сброс, выходим */
                    if(is_reset) {
                        reset_connections();
                        std::this_thread::yield();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        return;
                    }

                    if(named_pipe_connected) {
                        /* создаем отдельный поток для приема и передачи сообщений */
                        std::lock_guard<std::recursive_mutex> lock(connections_mutex);
                        connections.push_back(std::make_shared<Connection>(
                            pipe,
                            on_open,
                            on_message,
                            on_close,
                            on_error,
                            config.buffer_size));
                    } else {
                        CloseHandle(pipe);
                    }

                    /* удаляем потоки, где соединение закрыто */
                    clear_connections();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::this_thread::yield();
                }
                reset_connections();
            });
            return true;
        }
    public:

        std::function<void(Connection*)> on_open;
        std::function<void(Connection*, const std::string &in_message)> on_message;
        std::function<void(Connection*)> on_close;
        std::function<void(Connection*, const std::error_code &)> on_error;

        /** \brief Конструктор класса сервера именованных каналов
         *
         * \param name Имя именованного канала
         * \param buffer_size Размер буфера для чтения и записи
         * \param timeout Время ожидания
         */
        NamedPipeServer(
            const std::string &name,
            const size_t buffer_size =1024,
            const size_t timeout = 0) {
            is_reset = false;
            is_error = false;
            is_connection = false;
            config.name = name;
            config.buffer_size = buffer_size;
            config.timeout = timeout;
        }

        /** \brief Запустить сервер
         */
        bool start() noexcept {
            return init(config);
        }

        /** \brief Остановить сервер
         */
        void stop() noexcept {
            is_reset = true;
            if(is_connection) {
                /* применяем лайфхак, чтобы разблочить функцию
                 * ConnectNamedPipe
                 */
                 HANDLE hack_pipe;
                 {
                     std::lock_guard<std::mutex> lock(connection_pipe_name_mutex);
                     hack_pipe = CreateFile(
                        (LPCSTR)connection_pipe_name.c_str(), // имя канала
                        GENERIC_READ |  // read and write access
                        GENERIC_WRITE,
                        0,              // no sharing
                        NULL,           // default security attributes
                        OPEN_EXISTING,  // opens existing pipe
                        0,              // default attributes
                        NULL);          // no template file
                }
                if(hack_pipe != INVALID_HANDLE_VALUE) {
                    CloseHandle(pipe);
                }
            }

            if(named_pipe_future.valid()) {
                try {
                    named_pipe_future.wait();
                    named_pipe_future.get();
                }
                catch(...) {}
            }
            reset_connections();
        }

        /** \brief Проверить наличие ошибки
         *
         * \return Вернет true, если есть ошибка
         */
        inline bool check_error() noexcept {
            return is_error;
        }

		/** \brief Отправить сообщение всем клиента
		 * \param out_message Сообщение
		 * \return Вернет true, если было хотя бы одно отправление
		 */
        inline bool send_all(const std::string &out_message) noexcept {
			std::lock_guard<std::recursive_mutex> lock(connections_mutex);
			bool is_send = false;
			auto it = connections.begin();
			while(it != connections.end()) {
                if(!it->get()->check_close()) {
                    it->get()->send(out_message);
                    is_send = true;
                }
                it++;
            }
            return is_send;
		}

        ~NamedPipeServer() {
            stop();
        }

        /** \brief Получить количество соединений
		 * \return Количество соединений
		 */
        inline size_t get_connections() noexcept {
            clear_connections();
            std::lock_guard<std::recursive_mutex> lock(connections_mutex);
            return connections.size();
        };
    };
}

#endif // NAMED_PIPE_SERVER_HPP_INCLUDED
