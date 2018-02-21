#define SerialX Serial

void setup()
{


/*
 * 2MBIT
 * 
 *  
  Serial.begin(1000000);
  UCSR0A |= (1 << U2X0);                                  // Double up UART      //
  UCSR0B |= (1 << RXEN0)  | (1 << TXEN0) | (1 << RXCIE0); // UART RX, TX enable  //
                                                          // RX Interrupt enable //
  UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);                // Asynchrous 8N1      //
  UBRR0H = 0;                                                                    //
  UBRR0L = 0;                                             // Baud Rate 2 MBit    //
*/



  //SerialX.begin(1000000);
  //SerialX.begin(2000000);
  
  //SerialX.begin(500000);
  //SerialX.begin(250000);  //--nefunguje funky a cp210x protoz 250000 neni standardni rychlost
  //SerialX.begin(115200*2);
  SerialX.begin(115200);

}
long int c=0;
void loop()
{
  SerialX.print("T;");
  SerialX.print(c++);
  SerialX.print(";84;");
  SerialX.print(millis());
  SerialX.println(";10070");
/*
  SerialX.print("32 ");
  SerialX.print(c++);
  SerialX.print(" 0 ");
  SerialX.print(millis());
  SerialX.println(" 0");
*/  
}
