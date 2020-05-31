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
   while(!pipe.open("my_server")) {
      Print("Канал не создан, повтор через 5 секунд...");
      Sleep(1000);
   }
   uint start=GetTickCount();
   //pipe.write("test-1");
   //pipe.write("test-2");
   //pipe.write("test-3");
   //pipe.write("test-4");
   //pipe.write("abcdef");
   pipe.write("ping");
   uint stop=GetTickCount();
   Print("Пересылка заняла"+IntegerToString(stop-start)+" [ms]");
   Print("Получено сообщение: ",pipe.read());
   pipe.close();
   return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason) {

}

void OnTick() {

}

void OnChartEvent(const int id,
                  const long &lparam,
                  const double &dparam,
                  const string &sparam) {

}

