#include <SPI.h>
#include "mcp_can.h"

#define SPI_CS_PIN  10 

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


int send_data[13] = {0, 0, 33, 50, 1, 24, 3, 0, 10, 50, 0, 0, 10}; 
// RPM, SPD, Odometer, Motor Temperator, GEAR, Total Voltage, CHARGING, SOC, Temp_bat_min, Tem_bat_Max, Power, Throttle Possition, Current
// RPM, SPD(km/h): 415
// Motor Temperator (oC): 406
// GEAR(D,N,R), power(NULL, DELIVERY, RECUP), Throttle Possition (%), Total Voltage (V): 1435
// CHARGING (ERROR, INIT, READY, STOP), POWER OF CHARGE (xem lại), SOC (%), Temp_bat_min(oC), Tem_bat_Max(oC): 1060
// Odometer: 1369

void setup()
{
    Serial.begin(115200);    
    pinMode(LED_BUILTIN, OUTPUT);
    
    while (CAN_OK != CAN.begin(CAN_1000KBPS))   // init can bus : baudrate = CAN_1000KBPS
    {
        delay(100);
    }
}


void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned long canid = CAN.getCanId();
        switch (canid)
        {
          /////////////////// Speed & RPM ///////////////////
          case 415: // 0x19F
          {
            uint16_t value19F = buf[2]<<4 | buf[3] >>4;
            int16_t Rpm = (value19F - 2000)*10;
            float Spd = (abs(Rpm)/float(7250))*float(80);

            send_data[0] = Rpm;
            send_data[1] = Spd;
            
            break;
          }

          /////////////////// Odometer ///////////////////
          case 1433: //0x599
          {            
            int odo1 = buf[0], odo2 = buf[3];
            int odo = odo1 + odo2;
            
            send_data[2] = odo;

            break;
          }

          /////////////////// Motor Temperator ///////////////////
          case 406: //0x196
          {
            uint16_t Temp = buf[5] -40;
            send_data[3] = Temp;

            break;
          }
         case 1435: //0x59B
         {
            /////////////////// GEAR ///////////////////
            uint8_t gear = buf[0];
            
            switch (gear)
            {
              case 128: { send_data[4]=0; break; } //0x80 - D
              case 32:  { send_data[4]=1; break; } //0x20 - N
              case 8:   { send_data[4]=2; break; } //0x08 - R
            }
            
            /////////////// POWER /////////////////////////
            uint32_t power = buf[2];
           
            if (88<=power<=163)
            {
              if (power = 100) { send_data[10]=0; } // NULL
              else if ( power > 100) { send_data[10]=1; } // DELIVERY
              else if ( power < 100) { send_data[10]=2; } // RECUP
            }
            
            ///////////// TPS //////////////////
            uint8_t tps = buf[3];
            float p_tps;
            if (0 <= tps <= 253) // 0x00 <= tps <= 0xFD
            {
              p_tps = abs(tps)/float(253) * float(100);
              send_data[11]=p_tps; 
            }
            
            //////////////// BRAKE PEDAL ///////////
            //
            
            ////////////// TOTAL VOLTAGE ///////////
            uint8_t c_volt = buf[5];
            uint8_t volt;
            
            if ( 0 <= c_volt <= 48 ) 
            { 
              volt  = c_volt/2;
              send_data[5]=volt;
            }
           break;
        }
        
        case 1060: // 0x424
        {
          ////////////// POWER OF CHARGE RECUP & DRIVE /////////////
//          uint8_t inc, outc;
//          uint16_t recup , drive;
//          inc = buf[2]; outc = buf[3];
//          recup = inc * 500;
//          drive = outc * 500; 
//          if (0 <= recup <= 30000)
//          {
//            SERIAL_PORT_MONITOR.print("Max Recup: ");SERIAL_PORT_MONITOR.println(recup);
//            
//          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERROR "); }
//          if ( 0 <= drive <= 30000)
//          {
//            SERIAL_PORT_MONITOR.print("Drive: ");SERIAL_PORT_MONITOR.println(drive);
//          } else {  SERIAL_PORT_MONITOR.println("Max Recup: ERRO "); }

          //////////////// TEMP BAT + SOH /////////
          int8_t tpbmi = buf[4], tpbmx = buf[7];
          int32_t temp_bat_min, temp_bat_max, soh_bat = buf[6];
          temp_bat_min= tpbmi - 40;
          temp_bat_max= tpbmx - 40;

          send_data[8]=temp_bat_min;
          send_data[9]=temp_bat_max;
          //-----------------------------------------------------------
          break;
      }
      case(341): // 0x155
      {
        //////////////////////buf0////charging/////////
//        int32_t val_charge = buf[0];
//        int32_t poc, aoc;
//        if (0<=val_charge<=7)
//        {
//          poc = val_charge *300;
//          aoc = val_charge * 5;
//          SERIAL_PORT_MONITOR.print("Power of charging: "       );SERIAL_PORT_MONITOR.print(poc     );SERIAL_PORT_MONITOR.println(" Wat")  ;
//          SERIAL_PORT_MONITOR.print("Current for charging: "    );SERIAL_PORT_MONITOR.print(aoc     );SERIAL_PORT_MONITOR.println(" A")    ;
//        }
        ///////////////////////////// Battery Current //////////////////////
        int16_t bit_c = buf[1] & 15;
        int16_t bit_t = buf[2];
        int16_t val_pull = (bit_c << 8) | bit_t;
        int16_t val_ampe = (2000 - val_pull) / 4;
        
        send_data[12] = val_ampe;
          
        ////////////////////// SOC ////////////////////
        uint16_t soc_5 = uint8_t(buf[4]) << 8, soc_6 = buf[5];
        
        float soc = ((soc_5 | soc_6) / float(400));
        if (0<=soc<=100) { send_data[7]=soc; }

        break;
       }
       
       case (1061): //0x425
       {
          //////////// CHARGING ///////////
          uint32_t ck_c = buf[0];

          switch (ck_c)
          {
          case 29: // 0x1D
          {  send_data[6]=0;  break; } // INIT
          case 36: // 0x24
          {  send_data[6]=1;  break; } // READY
          case 10: // 0x0A
          {  send_data[6]=2;  break; } // CHARGING
          case 42: // 0x2A
          {  send_data[6]=3;  break; } // DRIVING
          case 44: // 0x2C
          {  send_data[6]=4;  break; } // START
          }
       }
    }
  }
  
  
  ///////// SEND DATA ///////
  char buffer[16];
  // if we get a command, turn the LED on or off:
  if (Serial.available() > 0) {
    int size = Serial.readBytesUntil('\n', buffer, 12);

    if (buffer[0] == 'Y') {
      digitalWrite(LED_BUILTIN, HIGH);
    }

    for (int i = 0; i < 13; i++)
    {
      Serial.print(send_data[i], DEC);
      Serial.print('\t');
    }
    Serial.println();
  }

  else digitalWrite(LED_BUILTIN, LOW);
  /////////////////////////
}

// END FILE
