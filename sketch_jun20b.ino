#include <SoftwareSerial.h>
#define SERIAL_BUFFER_SIZE 1024
SoftwareSerial mySerial(13, 12); // RX, TX 通过串口连接ESP8266
String incomingByte = " ";

unsigned int tempMin = 29; // 亮灯温度
unsigned int tempMax = 29 ; // 报警温度

void setup() {
  Serial.begin(115200); // 初始化串口通信，波特率设置为115200
  analogReference(INTERNAL); // 调用板载1.1V基准源
  pinMode(11, OUTPUT); // 将11端口设置为输出，初始状态关闭发光二极管

  // 初始化 ESP8266 模块连接 WiFi 并建立 TCP 连接
  mySerial.begin(115200); // 初始化软串口通信，波特率设置为115200
  mySerial.println("AT+RST"); // 初始化一次ESP8266
  delay(3000);
  echo();
  mySerial.println("AT");
  echo();
  mySerial.println("AT+CWMODE=3"); // 设置WIFI模式为 STA+AP 模式
  echo();
  delay(7000);
  mySerial.println("AT+CWJAP=\"vwo50\",\"00000000\""); // 连接 WiFi 网络
  echo();
  delay(8000);

}

void loop() {
  mySerial.println("AT+CIPMODE=1"); // 配置为透传模式
  echo();
  mySerial.println("AT+CIPSTART=\"TCP\",\"8.140.199.47\",6598"); // 连接服务器
  delay(2000);
  echo();
  double analogVotage = 1.1 * (double)analogRead(A3) / 1023;
  double temp = 100 * analogVotage; // 计算温度
  unsigned int dutyCycle; // 占空比
  sendTemperatureData(temp); // 将温度数据发送到服务器
  if (temp <= tempMin) { // 如果温度低于亮灯温度
    dutyCycle = 0; digitalWrite(11, LOW);
  }
  else if (temp < tempMax) { // 如果温度低于报警温度
    dutyCycle = (temp - tempMin) * 255 / (tempMax - tempMin);
    digitalWrite(11, LOW);
  }
  else { // 如果温度高于报警温度
    dutyCycle = 255; digitalWrite(11, HIGH);
    
  }
  analogWrite(10, dutyCycle); // 控制发光二极管发光
Serial.print("Temp: "); Serial.print(temp);
  Serial.print(" Degrees Duty cycle: ");
  Serial.println(dutyCycle);
  delay(100); // 控制刷新速度
}

void sendTemperatureData(double temp) {
  mySerial.println("AT+CIPSEND"); // 进入透传模式并准备发送数据
  echo();
  delay(400);
  mySerial.print("Tempeture");
  mySerial.print(temp);
  mySerial.print("Wet--Light---Pressure---kPa"); // 发送温度到服务器
  delay(8000);
  echo();
  mySerial.print("+++"); // 推出透传模式
  delay(4000);
}

void echo() {
  delay(500);
  while (mySerial.available()) {
    Serial.write(char(mySerial.read()));
    delay(60);
  }
}
