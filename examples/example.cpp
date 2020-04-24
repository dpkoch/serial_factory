#include <iostream>

#include "serial_factory.h"

struct SERIAL_MESSAGE FirstMessage
{
  bool flag;
  uint16_t data;

  friend std::ostream &operator<<(std::ostream &os, const FirstMessage &msg)
  {
    os << "flag: " << msg.flag << ", data: " << msg.data;
  }
};

struct SERIAL_MESSAGE SecondMessage
{
  uint32_t id;
  double data1;
  double data2;

  friend std::ostream &operator<<(std::ostream &os, const SecondMessage &msg)
  {
    os << "id: " << msg.id << ", data1: " << msg.data1 << ", data2: " << msg.data2;
  }
};


struct SERIAL_MESSAGE ThirdMessage
{
  uint32_t id;
  uint32_t data;

  friend std::ostream &operator<<(std::ostream &os, const ThirdMessage &msg)
  {
    os << "id: " << msg.id << ", data: " << msg.data;
  }
};

using mySerial = serial_factory::SerialFactory<FirstMessage, SecondMessage, ThirdMessage>;

int main()
{
  uint8_t buffer[mySerial::BUFFER_SIZE];

  SecondMessage out;
  out.id = 42;
  out.data1 = 245.62;
  out.data2 = 63.367;

  mySerial::send_to_buffer(buffer, out);

  mySerial::Parser parser;
  mySerial::GenericMessage msg;

  FirstMessage fm;
  SecondMessage sm;
  ThirdMessage tm;

  for (size_t i = 0; i < sizeof(buffer); i++)
  {
    if (parser.parse_byte(buffer[i], msg))
    {
      std::cout << "Got a message!" << std::endl;
      switch (msg.id)
      {
      case mySerial::id<FirstMessage>():
        std::cout << "Type is FirstMessage" << std::endl;
        fm = msg.unpack<FirstMessage>();
        std::cout << fm << std::endl;
        break;
      case mySerial::id<SecondMessage>():
        std::cout << "Type is SecondMessage" << std::endl;
        sm = msg.unpack<SecondMessage>();
        std::cout << sm << std::endl;
        break;
      case mySerial::id<ThirdMessage>():
        std::cout << "Type is ThirdMessage" << std::endl;
        tm = msg.unpack<ThirdMessage>();
        std::cout << tm << std::endl;
        break;
      }
    }
  }

  return 0;
}