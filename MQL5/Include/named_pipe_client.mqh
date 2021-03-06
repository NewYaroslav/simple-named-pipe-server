//+------------------------------------------------------------------+
//|                                            named_pipe_client.mqh |
//|                                      Copyright 2021, Elektro Yar |
//|          https://github.com/NewYaroslav/simple-named-pipe-server |
//+------------------------------------------------------------------+
#property copyright "Copyright 2021, Elektro Yar"
#property link      "https://github.com/NewYaroslav/simple-named-pipe-server"

#include <Files\FilePipe.mqh>

//+------------------------------------------------------------------+
//| DLL imports                                                      |
//+------------------------------------------------------------------+
#import "kernel32.dll"
ulong CreateNamedPipe(string pipeName,int openMode,int pipeMode,int maxInstances,int outBufferSize,int inBufferSize,int defaultTimeOut,int security);
int WaitNamedPipeW(string lpNamedPipeName,int nTimeOut);
bool ConnectNamedPipe(ulong pipeHandle,int overlapped);
bool DisconnectNamedPipe(ulong pipeHandle);
ulong CreateFileW(string name,int desiredAccess,int SharedMode,int security,int creation,int flags,int templateFile);
//int WriteFile(ulong fileHandle,short &buffer[],int bytes,int &numOfBytes,int overlapped);
int WriteFile(ulong fileHandle,uchar &buffer[],int bytes,int &numOfBytes,int overlapped);
//int WriteFile(ulong fileHandle,MqlTick &outgoing,int bytes,int &numOfBytes,int overlapped);
//int WriteFile(ulong fileHandle,int &var,int bytes,int &numOfBytes,int overlapped);
int ReadFile(ulong fileHandle,short &buffer[],int bytes,int &numOfBytes,int overlapped);
int ReadFile(ulong fileHandle,char &buffer[],int bytes,int &numOfBytes,int overlapped);
int ReadFile(ulong fileHandle,MqlTick &incoming,int bytes,int &numOfBytes,int overlapped);
int ReadFile(ulong fileHandle,int &incoming,int bytes,int &numOfBytes,int overlapped);
int CloseHandle(ulong fileHandle);
int GetLastError(void);
int FlushFileBuffers(ulong pipeHandle);
bool SetNamedPipeHandleState(ulong fileHandle,int &lpMode, int lpMaxCollectionCount,int lpCollectDataTimeout);
bool PeekNamedPipe(int fileHandle, int buffer, int bytes, int bytesRead, int &numOfBytes, int bytesLeftThisMessage);
#import

enum ENUM_PIPE_ACCESS {
	PIPE_ACCESS_INBOUND = 1,
	PIPE_ACCESS_OUTBOUND = 2,
	PIPE_ACCESS_DUPLEX = 3,
};

enum ENUM_PIPE_MODE {
	PIPE_TYPE_RW_BYTE = 0,
	PIPE_TYPE_READ_MESSAGE = 2,
	PIPE_TYPE_WRITE_MESSAGE = 4,
};

#define PIPE_WAIT 0
#define PIPE_NOWAIT 1
#define PIPE_READMODE_MESSAGE 2

#define ERROR_PIPE_CONNECTED 535
#define ERROR_BROKEN_PIPE 109

#define INVALID_HANDLE_VALUE -1
#define GENERIC_READ  0x80000000
#define GENERIC_WRITE  0x40000000
#define OPEN_EXISTING  3
#define PIPE_UNLIMITED_INSTANCES 255
#define PIPE_BUFFER_SIZE 4096
#define STR_SIZE 255


class CFilePipeModified : public CFilePipe {
public:
   CFilePipeModified(void) {};
   ~CFilePipeModified(void) {};
  
   ulong GetBytesRead() {
      if (m_handle == INVALID_HANDLE || IsStopped()) return 0;
      return FileSize(m_handle);
   }
};


/** \brief Класс клиента именованного канала
 */
class NamedPipeClient {
private:
   CFilePipeModified ExtPipe;
   bool is_init;
	string pipeNumber;
	string pipe_name_prefix;
	int BufferSize;

protected:

public:

	/** \brief Конструктор класса
	 */
	NamedPipeClient() {
		pipe_name_prefix = "\\\\.\\pipe\\";
		is_init = false;
		BufferSize = 2048;
	}
	
	/** \brief Деструктор класса
	 */
	~NamedPipeClient() {
		if (is_init) ExtPipe.Close();
	}
	
	/** \brief Установить размер буфера
	 * \param size размер буфера
	 */ 
	void set_buffer_size(int size) {
	   BufferSize = size;
	}

	/** \brief Открывает канал, открытый ранее
	 * \param name Имя именнованного канала
	 * \return Вернет true - если успешно, иначе false
	 */ 
	bool open(string name) {
		const string full_pipe_name = pipe_name_prefix + name;

		if(!is_init) {
			if(WaitNamedPipeW(full_pipe_name, 5000) == 0) {
				//Print("Pipe " + name + " busy.");
				return false;
			}
			
			if (ExtPipe.Open(full_pipe_name , FILE_READ | FILE_WRITE | FILE_BIN ) == INVALID_HANDLE) {
			   Print("Pipe " + name + " open failed");
			   return false;
			}

			/* устанавливаем режим чтения
          * Клиентская сторона именованного канала начинается в байтовом режиме,
          * даже если серверная часть находится в режиме сообщений.
          * Чтобы избежать проблем с получением данных,
          * установите на стороне клиента также режим сообщений.
          * Чтобы изменить режим канала, клиент канала должен
          * открыть канал только для чтения с доступом
          * GENERIC_READ и FILE_WRITE_ATTRIBUTES
          */
          /*
         int mode = PIPE_READMODE_MESSAGE;
         bool success = SetNamedPipeHandleState(
            hPipe,
            mode,
            NULL,     // не устанавливать максимальные байты
            NULL);    // не устанавливайте максимальное время
         
         if(!success) {
            Print("SetNamedPipeHandleState failed");
            CloseHandle(hPipe);
				return false;
         }
         */
         is_init = true;
		}
		return true;
	}

	/** \brief Закрывает хэндл канала
	 * \return true если успешно, иначе ненулевое значение  
	 */ 
	bool close() {
		if (is_init) {
		   ExtPipe.Close();
		   is_init = false;
		   return true;
		} 
		return false;
	}

	/** \brief Сбрасывает буфер канала  
	 */
	void flush() {
		//FlushFileBuffers(hPipe);
	}
	
	/** \brief Записывает строку формата ANSI в канал
	 * \param message Строка, содержащее сообщение
	 * \return Вернет true, если запись прошла успешно
	 */
	bool write(string message) {
		if (!is_init) return false;
		uchar ansi_array[];
		int bytes_to_write = StringToCharArray(message, ansi_array);
		ulong bytes_written = ExtPipe.WriteArray(ansi_array);
		if (bytes_written != ArraySize(ansi_array)) return false;
		return true;
	}
   
	/** \brief Читает строку формата ANSI из канала 
	 * \return строка в формате Unicode (string в MQL5)
	 */
	string read() {
		string ret;
		if (!is_init) return ret;
		ExtPipe.ReadString(ret, (int)get_bytes_read());
		return ret;
	}
	
	/** \brief Получить количество байтов для чтения
	 * \return количество байтов
	 */
	ulong get_bytes_read() {
      return ExtPipe.GetBytesRead();
	}

	/** \brief Возвращает имя канала
	 * \return трока с именем канала
	 */
	string get_pipe_name() {
		return pipeNumber;
	}
};
