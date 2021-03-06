//+------------------------------------------------------------------+
//|                                       test_named_pipe_client.mq4 |
//|                         Copyright 2019-2020, Yaroslav Barabanov. |
//|                                https://t.me/BinaryOptionsScience |
//+------------------------------------------------------------------+
#property copyright "Copyright 2019-2020, Yaroslav Barabanov."
#property link      "https://t.me/BinaryOptionsScience"
#property version   "1.00"
#property strict

#include <named_pipe_client.mqh>
NamedPipeClient pipe;

int OnInit() {
   EventSetMillisecondTimer(10);
   while(!pipe.open("my_server")) {
      Print("Канал не создан, повтор через 1 секунд...");
      Sleep(1000);
   }
   uint start=GetTickCount();
   pipe.write("ping");
   uint stop=GetTickCount();
   Print("Пересылка заняла"+IntegerToString(stop-start)+" [ms]");
   while(pipe.get_bytes_read() == 0) {
      Sleep(1);
   }
   Print("Получено сообщение: ",pipe.read());
   return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason) {
   EventKillTimer();
   pipe.close();
}

void OnTimer() {
   
}

