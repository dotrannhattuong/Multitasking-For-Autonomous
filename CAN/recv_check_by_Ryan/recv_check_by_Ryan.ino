/*  receive a frame from can bus
    support@longan-labs.cc
   
    CAN Baudrate,
    
    #define CAN_5KBPS           1
    #define CAN_10KBPS          2
    #define CAN_20KBPS          3
    #define CAN_25KBPS          4 
    #define CAN_31K25BPS        5
    #define CAN_33KBPS          6
    #define CAN_40KBPS          7
    #define CAN_50KBPS          8
    #define CAN_80KBPS          9
    #define CAN_83K3BPS         10
    #define CAN_95KBPS          11
    #define CAN_100KBPS         12
    #define CAN_125KBPS         13
    #define CAN_200KBPS         14
    #define CAN_250KBPS         15
    #define CAN_500KBPS         16
    #define CAN_666KBPS         17
    #define CAN_1000KBPS        18

    CANBed V1: https://www.longan-labs.cc/1030008.html
    CANBed M0: https://www.longan-labs.cc/1030014.html
    CAN Bus Shield: https://www.longan-labs.cc/1030016.html
    OBD-II CAN Bus GPS Dev Kit: https://www.longan-labs.cc/1030003.html
*/

#include <SPI.h>
#include "mcp_can.h"

/* Please modify SPI_CS_PIN to adapt to different baords.

   CANBed V1        - 17
   CANBed M0        - 3
   CAN Bus Shield   - 9
   CANBed 2040      - 9
   CANBed Dual      - 9
   OBD-2G Dev Kit   - 9
   OBD-II GPS Kit   - 9
   Hud Dev Kit      - 9
*/


#define SPI_CS_PIN  10 

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


void setup()
{
    Serial.begin(115200);
    while(!Serial);
    
    // below code need for OBD-II GPS Dev Kit Atemga32U4 version
    // pinMode(A3, OUTPUT);
    // digitalWrite(A3, HIGH);
    
    // below code need for OBD-II GPS Dev Kit RP2040 version
    // pinMode(12, OUTPUT);
    // digitalWrite(12, HIGH);
    
    while (CAN_OK != CAN.begin(CAN_1000KBPS))    // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS FAIL!");
        delay(100);
    }
    Serial.println("CAN BUS OK!");
}


void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned long canid = CAN.getCanId();
        
//        Serial.println("-----------------------------");
        Serial.print("Get data from ID: ");
        Serial.println(canid, HEX);
//
        for(int i = 0; i<len; i++)    // print the data
        {
            Serial.print(buf[i]);
            Serial.print("\t");
        }
        Serial.println();
        switch (canid)
        {
          case 415: // 0x19F
          {
            uint16_t value19F = buf[2]<<4 | buf[3] >>4;
            int Rpm = (value19F - 2000)*10;                               // int32_t = int: in ra một số tự nhiên
            float Spd = (abs(Rpm)/float(7250))*float(80);
            SERIAL_PORT_MONITOR.print("value: "); SERIAL_PORT_MONITOR.println(value19F);
            
            SERIAL_PORT_MONITOR.print("Speed of Motor: ");  SERIAL_PORT_MONITOR.print(Rpm);  SERIAL_PORT_MONITOR.println(" Rpm");
            
            SERIAL_PORT_MONITOR.print("Speed of Vehicle: ");  SERIAL_PORT_MONITOR.print(Spd); SERIAL_PORT_MONITOR.println(" Km/h");
           
            //--------------------------------------------
            break;
          }
          case 406: //0x196
          {
            uint16_t Temp = buf[5] -40;
            SERIAL_PORT_MONITOR.print("Motor Temperator: ");  SERIAL_PORT_MONITOR.print(Temp);   SERIAL_PORT_MONITOR.println(" (C)");
            //----------------------------------------------
            break;
          }
         case 1435: //0x59B
         {
            ///////////////////gear///////////////////
            uint8_t gear = buf [0];
            SERIAL_PORT_MONITOR.print("Gear Selection: ");
            switch (gear)
            {
              case 128: { SERIAL_PORT_MONITOR.println("D"); break; }  //0x80
              case 32:  { SERIAL_PORT_MONITOR.println("N"); break; }  //0x20
              case 8:   { SERIAL_PORT_MONITOR.println("R"); break; }  //0x08
            }
            ///////////////power/////////////////////////
            uint32_t power = buf[2];
            SERIAL_PORT_MONITOR.print ("Power Status: ");
            if (88<=power<=163)
            {
              if (power == 100) {SERIAL_PORT_MONITOR.println("NULL");}
              else if ( power > 100) {SERIAL_PORT_MONITOR.println("DELIVERY");}
              else if ( power < 100) {SERIAL_PORT_MONITOR.println("RECUP");}
            }
            /////////////TPS//////////////////
            uint8_t tps = buf[3];
            float p_tps;
            SERIAL_PORT_MONITOR.print("Throttle Possition: ");
            if (0 <= tps <= 253) // 0x00 <= tps <= 0xFD
            {
              p_tps = abs(tps)/float(253) * float(100);    
              SERIAL_PORT_MONITOR.print(p_tps); SERIAL_PORT_MONITOR.println(" %");       
            }
            else {  SERIAL_PORT_MONITOR.println(" CAN GET TPS");}
            ////////////////BRAKE PEDAL///////////
            //
            //////////////TOTAL VOLTAGE///////////
            uint8_t c_volt = buf[5];
            int volt;
            if ( 0 <= c_volt <= 48 ) 
            { 
              volt  = c_volt /2; 
              SERIAL_PORT_MONITOR.print("Total Voltage: "); 
              SERIAL_PORT_MONITOR.print(volt); 
              SERIAL_PORT_MONITOR.println(" V");           
            }
            else { SERIAL_PORT_MONITOR.println("Total Voltage: CHECK ERROR "); }  
            //------------------------------------------------------------
            break;
        }
        case 1060: // 0x424
        {
//          ////////////CHARGING///////////
//          uint32_t ck_c = buf[0];
//          SERIAL_PORT_MONITOR.print("Checking Charging Battery: ");
//          switch (ck_c)
//          {         
//          case 9: // 0x9
//          {   SERIAL_PORT_MONITOR.println(" ERROR"); break;  }
//          case 0: // 0x0
//          {   SERIAL_PORT_MONITOR.println(" INIT");  break;  }
//          case 17: // 0x11
//          {   SERIAL_PORT_MONITOR.println(" READY"); break;  }
//          case 18: // 0x12
//          {   SERIAL_PORT_MONITOR.println(" STOP");  break;  }
//          }
          //////////////POWER OF CHARGE (RECUP & DRIVE/////////////
          uint8_t inc, outc;
          uint16_t recup , drive;
          inc = buf[2]; outc = buf[3];
          recup = inc * 500;
          drive = outc * 500; 
          if (0 <= recup <= 30000)
          {
            SERIAL_PORT_MONITOR.print("Max Recup: ");SERIAL_PORT_MONITOR.println(recup);
            
          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERROR "); }
          if ( 0 <= drive <= 30000)
          {
            SERIAL_PORT_MONITOR.print("Drive: ");SERIAL_PORT_MONITOR.println(drive);
          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERRO "); }
          //////////////// TEMP BAT + SOC /////////
          int8_t tpbmi = buf [4], tpbmx = buf [7];
          int32_t temp_bat_min,temp_bat_max, soh_bat = buf[5]  ;
          temp_bat_min= tpbmi - 40; 
          temp_bat_max= tpbmx - 40;
          SERIAL_PORT_MONITOR.print("Battery State Of Health: "    );SERIAL_PORT_MONITOR.print(soh_bat     );SERIAL_PORT_MONITOR.println(" %")  ;
          SERIAL_PORT_MONITOR.print("Battery Temp Min: "           );SERIAL_PORT_MONITOR.print(temp_bat_min);SERIAL_PORT_MONITOR.println(" (C)");
          SERIAL_PORT_MONITOR.print("Battery Temp Max: "           );SERIAL_PORT_MONITOR.print(temp_bat_max);SERIAL_PORT_MONITOR.println(" (C)");
          //-----------------------------------------------------------
          break;
      }
      case(341): // 0x155
      {
        //////////////////////buf0////charging/////////
        int32_t val_charge = buf[0];
        int32_t poc, aoc;
        if (0<=val_charge<=7)
        {
          poc = val_charge *300;
          aoc = val_charge * 5;
          SERIAL_PORT_MONITOR.print("Power of charging: "       );SERIAL_PORT_MONITOR.print(poc     );SERIAL_PORT_MONITOR.println(" Wat")  ;
          SERIAL_PORT_MONITOR.print("Current for charging: "    );SERIAL_PORT_MONITOR.print(aoc     );SERIAL_PORT_MONITOR.println(" A")    ;
        }
        /////////////////////////////battery current//////////////////////
        int16_t bit_c = buf[1] & 15;
        int16_t bit_t = buf[2];
        int16_t val_pull = (bit_c << 8) | bit_t;
        int16_t val_ampe = (val_pull - 2000) / 4;
        if      (val_ampe > 0)  {SERIAL_PORT_MONITOR.print("current Consumsion: ")                      ; SERIAL_PORT_MONITOR.print(val_ampe);SERIAL_PORT_MONITOR.println(" A")  ; }
        else if (val_ampe < 0)  {SERIAL_PORT_MONITOR.print("Current Recharge for Battery: ")            ; SERIAL_PORT_MONITOR.print(val_ampe);SERIAL_PORT_MONITOR.println(" A")  ; }
        else if (val_ampe = 0)  {SERIAL_PORT_MONITOR.println("NONE CURRENT")                            ; }
        //////////////////////////////////////////CAN STATUS//////////////////////////
        int can =  buf[3];
        switch (can)
        {
          case 148:
          {
            SERIAL_PORT_MONITOR.println("CAN NETWORK: OFF"); break;
          }
          case 84:
          {
            SERIAL_PORT_MONITOR.println("CAN NETWORK: ON"); break;
          }
          default:
          {
            SERIAL_PORT_MONITOR.println("CHECKNG CAN NETWORK"); break;
          }
        }
        //////////////////////SOC////////////////////
        uint16_t soc_5 = buf[4]<<8, soc_6 = buf[5];
        float soc = abs((soc_5 | soc_6) / float(400));
        if (0<=soc<=100) {SERIAL_PORT_MONITOR.print("State Of Charging: ");SERIAL_PORT_MONITOR.print(soc);SERIAL_PORT_MONITOR.println(" %"); }
        else {SERIAL_PORT_MONITOR.println("State Of Charging: ERROR");}
        ///////////////////////////////////////////////////////
        break;
       }
       case (1061): //0x425
       {
        ////////////CHARGING///////////
         uint32_t ck_c = buf[0];
         SERIAL_PORT_MONITOR.print("Checking Charging Battery: ");
         switch (ck_c)
         {         
         case 29: // 0x1A
         {   SERIAL_PORT_MONITOR.println(" INIT"); break;  }
         case 36: // 0x24
         {   SERIAL_PORT_MONITOR.println(" READY");  break;  }
         case 10: // 0x0A
         {   SERIAL_PORT_MONITOR.println(" CHARGING"); break;  }
         case 42: // 0x2A
         {   SERIAL_PORT_MONITOR.println(" DRIVING");  break;  }
         case 44: // 0x2C
         {   SERIAL_PORT_MONITOR.println(" START TRICKLE CHARGING");  break;  }
         }
        
        ///////////////////////////////
        break;
       }
       case (1366): //0x556
       {
        ///////////////////////////////
        break;
       }
       case (1433): //0x599
       {
        int odo1 = buf[0], odo2 = buf[3];
        int odo = odo1 + odo2;
        SERIAL_PORT_MONITOR.print("Odo Meter: ");SERIAL_PORT_MONITOR.print(odo);SERIAL_PORT_MONITOR.println(" Km");
        ///////////////////////////////
        break;
       }
       case (1364): //0x554
       {
        ///////////////////////////////
        break;
       }
       case (1367): //0x557
       {
        ///////////////////////////////
        break;
       }
       case (1374): //0x55E
       {
        uint16_t redis7 = buf[6], redis8 = buf[7];
        
        ///////////////////////////////
        break;
       }
       case (1375): //0x55F
       {
        
        ///////////////////////////////
        break;
       }
    }
  }     
}

// END FILE
