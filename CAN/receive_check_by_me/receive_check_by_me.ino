
#include <SPI.h>
#define CAN_2515
// #define CAN_2518FD

// Set SPI CS Pin according to your hardware

#if defined(SEEED_WIO_TERMINAL) && defined(CAN_2518FD)
// For Wio Terminal w/ MCP2518FD RPi Hat：
// Channel 0 SPI_CS Pin: BCM 8
// Channel 1 SPI_CS Pin: BCM 7
// Interupt Pin: BCM25
const int SPI_CS_PIN  = BCM8;
const int CAN_INT_PIN = BCM25;
#else

// For Arduino MCP2515 Hat:
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;
const int CAN_INT_PIN = 2;
#endif


#ifdef CAN_2518FD
#include "mcp2518fd_can.h"
mcp2518fd CAN(SPI_CS_PIN); // Set CS pin
#endif

#ifdef CAN_2515
#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin
#endif                           

void setup() {
    SERIAL_PORT_MONITOR.begin(115200);

    while (CAN_OK != CAN.begin(CAN_1000KBPS)) {             // init can bus : baudrate = 1000
        SERIAL_PORT_MONITOR.println("CAN init fail, retry...");
        delay(100);
    }
    SERIAL_PORT_MONITOR.println("CAN init ok!");
}

void loop() 
{
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == CAN.checkReceive()) {         // check if data coming
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned long canId = CAN.getCanId();

        SERIAL_PORT_MONITOR.println("-----------------------------");
        SERIAL_PORT_MONITOR.print("Get data from ID: 0x");
        SERIAL_PORT_MONITOR.println(canId, HEX);

        for (int i = 0; i < len; i++) { // print the data
            SERIAL_PORT_MONITOR.print(buf[i], HEX);
            SERIAL_PORT_MONITOR.print("\t");
        }
        SERIAL_PORT_MONITOR.println();
        ////////////////CAN SOLUTION////////////////////
        switch (canId)
        {
          case 415: // 0x19F
          {
            uint16_t value19F = buf[2]<<4 | buf[3] >>4;
            uint16_t Rpm = (value19F - 2000)*10;
            uint16_t Spd = (Rpm/7250)*80;
            SERIAL_PORT_MONITOR.println("Speed of Motor: "); //seRpm, " RPM");
            SERIAL_PORT_MONITOR.print(Rpm);
            SERIAL_PORT_MONITOR.print(" Rpm");
            SERIAL_PORT_MONITOR.println("Speed of Vehicle: "); //,Spd," Km/h");
            SERIAL_PORT_MONITOR.print(Spd);
            SERIAL_PORT_MONITOR.print(" Km/h");
            //--------------------------------------------
            break;
          }
          case 406: //0x196
          {
            uint16_t Temp = buf[5] -40;
            SERIAL_PORT_MONITOR.println("Motor Temperator: "); 
            SERIAL_PORT_MONITOR.print(Temp);
            SERIAL_PORT_MONITOR.print(" (C)");
            //----------------------------------------------
            break;
          }
         case 1435: //0x59B
         {
            ///////////////////gear///////////////////
            uint8_t gear = buf [0];
            SERIAL_PORT_MONITOR.println("Gear Selection: ");
            switch (gear)
            {
              case 128: { SERIAL_PORT_MONITOR.print("D"); break; }  //0x80
              case 32:  { SERIAL_PORT_MONITOR.print("N"); break; }  //0x20
              case 8:   { SERIAL_PORT_MONITOR.print("R"); break; }  //0x08
            }
            ///////////////power/////////////////////////
            uint8_t power = buf[2];
            SERIAL_PORT_MONITOR.println ("Power Status: ");
            if (88<=power<=163)
            {
              if (power = 100) {SERIAL_PORT_MONITOR.print("NULL");}
              else if ( power > 100) {SERIAL_PORT_MONITOR.print("DELIVERY");}
              else if ( power < 100) {SERIAL_PORT_MONITOR.print("RECUP");}
            }
            /////////////TPS//////////////////
            uint8_t tps = buf[3];
            uint8_t p_tps;
            SERIAL_PORT_MONITOR.println("Throttle Possition: ");
            if (0 <= tps <= 253) // 0x00 <= tps <= 0xFD
            {
              p_tps = (tps/253) * 100;    
              SERIAL_PORT_MONITOR.print(p_tps); SERIAL_PORT_MONITOR.print(" %");       
            }
            else {  SERIAL_PORT_MONITOR.print(" CAN GET TPS");}
            ////////////////BRAKE PEDAL///////////
            //
            //////////////TOTAL VOLTAGE///////////
            uint8_t c_volt = buf[5];
            uint8_t volt;
            if ( 0 <= c_volt <= 48 ) 
            { 
              volt  = c_volt /2; 
              SERIAL_PORT_MONITOR.println("Total Voltage: "); 
              SERIAL_PORT_MONITOR.print(volt); 
              SERIAL_PORT_MONITOR.print (" V");           
            }
            else { SERIAL_PORT_MONITOR.println("Total Voltage: CHECK ERROR "); }
            //------------------------------------------------------------
            break;
        }
        case 1060: // 0x424
        {
          ////////////CHARGING///////////
          uint8_t ck_c = buf[0];
          switch (ck_c);
          SERIAL_PORT_MONITOR.println("Checking Charging Battery: ");
          case 9: // 0x9
          {   SERIAL_PORT_MONITOR.print(" ERROR"); break;  }
          case 0: // 0x0
          {   SERIAL_PORT_MONITOR.print(" INIT");  break;  }
          case 17: // 0x11
          {   SERIAL_PORT_MONITOR.print(" READY"); break;  }
          case 18: // 0x12
          {   SERIAL_PORT_MONITOR.print(" STOP");  break;  }
          //////////////POWER OF CHARGE (RECUP & DRIVE/////////////
          uint8_t inc, outc;
          uint16_t recup , drive;
          inc = buf[2]; outc = buf[3];
          recup = inc * 500;
          drive = outc * 500; 
          if (0 <= recup <= 3000)
          {
            SERIAL_PORT_MONITOR.println("Max Recup: ");SERIAL_PORT_MONITOR.print(recup);
            
          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERRO "); }
          if ( 0 <= drive <= 3000)
          {
            SERIAL_PORT_MONITOR.println("Drive: ");SERIAL_PORT_MONITOR.print(drive);
          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERRO "); }
          //////////////// TEMP BAT + SOC /////////
          uint8_t tpbmi = buf [4], tpbmx = buf [7], soc_bat = buf[6];
          uint8_t temp_bat_min,temp_bat_max  ;
          temp_bat_min= tpbmi - 40;
          temp_bat_max= tpbmx - 40;
          SERIAL_PORT_MONITOR.println("Battery State Of Charge: "    );SERIAL_PORT_MONITOR.print(soc_bat     );
          SERIAL_PORT_MONITOR.println("Battery Temp Min: "           );SERIAL_PORT_MONITOR.print(temp_bat_min);
          SERIAL_PORT_MONITOR.println("Battery Temp Max: "           );SERIAL_PORT_MONITOR.print(temp_bat_max);
          //-----------------------------------------------------------
          break;
      }
    }
  }
}

/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
