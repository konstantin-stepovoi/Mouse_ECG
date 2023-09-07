#define  MAX_BRIGHTNESS 255                         //  Задаём переменную максимальной яркости свечения светодиода

#include "Wire.h"                                   //  Подключаем библиотеку для работы с шиной I2C
#include "MAX30105.h"                               //  Подключаем библиотеку для работы с модулем
#include "spo2_algorithm.h"                         //  Подключаем блок работы с насыщением крови кислородом
MAX30105 PARTICLE_SENSOR;                           //  Создаём объект для работы с библиотекой

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100];                             //  16-битный массив данных от сенсора со значениями от ИК-светодиода
uint16_t redBuffer[100];                            //  16-битный массив данных от сенсора со значениями от красного светодиода
#else
uint32_t irBuffer[100];                             //  32-битный массив данных от сенсора со значениями от ИК-светодиода
uint32_t redBuffer[100];                            //  32-битный массив данных от сенсора со значениями от красного светодиода
#endif

int32_t bufferLength;                               //  длина буфера данных
int32_t spo2;                                       //  значение SpO2 (насыщенности крови кислородом)
int8_t  validSPO2;                                  //  флаг валидности значений сенсора по SpO2
int32_t heartRate;                                  //  значение ЧСС
int8_t  validHeartRate;                             //  флаг валидности значений сенсора по ЧСС

void setup() {
  Serial.begin(115200);                             //  инициируем работу с монитором последовательного порта на скорости 115200 бод
  if (!PARTICLE_SENSOR.begin()) {                   //  инициируем работу с сенсором. Если этого не произошло, то
    Serial.println(F("MAX30105 was not found."));   //  Выводим сообщением об этом и
    while (1);                                      //  останавливаем дальнейшее выполнение скетча
  }
  Serial.println(F("Press any key!"));              //  Прежде, чем переходить к считыванию, выводим в монитор порта сообщение и
  while (Serial.available() == 0);                  //  ждём отправки любого символа в монитор порта (нажмите Enter, находясь в строке отправки)
  Serial.read();                                    //  Если символ получен, то скетч будет выполнен дальше

  byte ledBrightness  = 60;                         //  Задаём яркость работы светодиода, при этом потребление тока будет следующим: 0 - 0мА, 255 - 50 мА
  byte sampleAverage  = 4;                          //  Устанавливаем коэффициент усреднения. Возможные варианты значений: 1, 2, 4, 8, 16, 32
  byte ledMode        = 2;                          //  Устанавливаем режим работы светодиодов на сенсоре: 1 - только красный (Red only), 2 - красный и ИК (Red + IR), 3 - красный, ИК и зелёный (Red + IR + Green)
  byte sampleRate     = 100;                        //  Устанавливаем частоту дискретизации (сглаживания сигнала). Варианты: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int  pulseWidth     = 411;                        //  Устанавливаем ширину импульса. Варианты: 69, 118, 215, 411
  int  adcRange       = 4096;                       //  Устанавливаем диапазон значений с АЦП. Варианты: 2048(11 бит), 4096(12 бит), 8192(13 бит), 16384(14 бит)

  PARTICLE_SENSOR.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop() {
  bufferLength = 100;                                 //  Устанавливаем длину буфера равным 100 (куда будут записаны пакеты по 25 значений в течении 4 секунд)
  for (byte i = 0 ; i < bufferLength ; i++) {         //  проходим в цикле по буферу и
    while (PARTICLE_SENSOR.available() == false)      //  отправляем сенсору запрос на получение новых данных
      PARTICLE_SENSOR.check();
    redBuffer[i] = PARTICLE_SENSOR.getRed();          //  Записываем в массив значения сенсора, полученные при работе с КРАСНЫМ светодиодом
    irBuffer[i] = PARTICLE_SENSOR.getIR();            //  Записываем в массив значения сенсора, полученные при работе с ИК      светодиодом
    PARTICLE_SENSOR.nextSample();                     //  Как только в буфер было записано 100 значений - отправляем сенсору команду начать вычислять значения ЧСС и SpO2
    Serial.print(F("red="));                          //  Выводим текст в монитор последовательного порта
    Serial.print(redBuffer[i], DEC);                  //  Выводим значение переменной redBuffer[i] в монитор последовательного порта
    Serial.print(F(", ir="));                         //  Выводим текст в монитор последовательного порта
    Serial.println(irBuffer[i], DEC);                 //  Выводим значение переменной irBuffer[i] в монитор последовательного порта
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  while (1) {
    //  Сбрасываем первые полученные 25 значений из буфера, а оставшиеся 75 сдвигаем влево в массиве
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }
    //  Получаем новые 25 значений прежде чем переходить к вычислению ЧСС
    for (byte i = 75; i < 100; i++) {
      while (PARTICLE_SENSOR.available() == false) {  //  Опрашиваем сенсор на предмет наличия новых значений
        PARTICLE_SENSOR.check();
      }
      redBuffer[i] = PARTICLE_SENSOR.getRed();        //  Записываем в массив значения сенсора, полученные при работе с КРАСНЫМ светодиодом
      irBuffer[i] = PARTICLE_SENSOR.getIR();          //  Записываем в массив значения сенсора, полученные при работе с ИК      светодиодом
      PARTICLE_SENSOR.nextSample();                   //  Как только в буфер было записано 100 значений - отправляем сенсору команду начать вычислять значения ЧСС и SpO2

      Serial.print(F("red="));                        //  Выводим текст в монитор последовательного порта
      Serial.print(redBuffer[i], DEC);                //  Выводим значение переменной   redBuffer[i]   в монитор последовательного порта
      Serial.print(F(", ir="));                       //  Выводим текст в монитор последовательного порта
      Serial.print(irBuffer[i], DEC);                 //  Выводим значение переменной   irBuffer[i]    в монитор последовательного порта
      Serial.print(F(", HR="));                       //  Выводим текст в монитор последовательного порта
      Serial.print(heartRate, DEC);                   //  Выводим значение переменной   heartRate      в монитор последовательного порта
      Serial.print(F(", HRvalid="));                  //  Выводим текст в монитор последовательного порта
      Serial.print(validHeartRate, DEC);              //  Выводим значение переменной   validHeartRate в монитор последовательного порта
      Serial.print(F(", SPO2="));                     //  Выводим текст в монитор последовательного порта
      Serial.print(spo2, DEC);                        //  Выводим значение переменной   spo2           в монитор последовательного порта
      Serial.print(F(", SPO2Valid="));                //  Выводим текст в монитор последовательного порта
      Serial.println(validSPO2, DEC);                 //  Выводим значение переменной   validSPO2      в монитор последовательного порта
    }
    //  После получения очередного пакета из 25 значений повторно считаем значения ЧСС и SpO2
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
}