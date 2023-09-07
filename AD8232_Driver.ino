// понадобится скачать и добавить в среду разработки Ардуино IDE специализированную библиотеку AD8232 Heart Rate Monitor
//(можно «взять» тут: https://github.com/sparkfun/AD8232_Heart_Rate_Monitor)
// Добавляем вручную в папку с программой C:\Program Files (x86)\Arduino\libraries\
// За инструкцию по подключению спасибо https://arduino-ide.com/modules/14-podkljuchenie-ad8232-k-arduino.html

void setup()
{
  Serial.begin(9600);
  // L0+ и L0-, подключенные на 10 и 11 пины, являются выходами детектора контактов электродов
  pinMode(10, INPUT); // Настройка выхода L0-
  pinMode(11, INPUT); // Настройка выхода L0+
}
void loop()
{
  // При отсутствии контакта на одном из 3 электродов
  // в мониторе порта будет отображаться "!" на месте
  // отображения значения этого электрода
  if((digitalRead(10) == 1)||(digitalRead(11) == 1))
  {
    Serial.println('!');
  }
  else{
    // отправка аналоговых значений с датчиков
      Serial.println(analogRead(A0));
  }
  // Небольшая задержка для получения значений с электродов
  delay(1);
}
