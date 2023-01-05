import serial
import numpy as np
import time 

class CAN:
    def __init__(self):
        self.__testing = False

        try:
            board = '/dev/ttyACM0'
            # board = 'COM7'
            baud_rate = 115200
            self.arduino = serial.Serial(board, baud_rate, timeout=10)
        except:
            self.__testing = True
        
        print(self.__testing)
        ############ SPEED VISUALIZE ############
        self.__speed = np.zeros(20)
        self.__time = np.zeros(20)
        self.__pre_time = time.time()

        ##########################################################
        self.__name_gear = ['D', 'N', 'R']
        self.__name_charging = ['INIT', 'READY', 'CHARGING', 'DRIVING', 'START']
        self.__motor_status = ["STOP", "START", "ERROR"]
        self.__brake = ['INACTIVE', 'ACTIVE', 'ERROR']

        ############ DEBUGING ############
        self.__pre_data = [0, 0, 33, 50, 'N', 24, 'OFF', 0, 33, 50, 'NULL', 0, 10, 70, "INACTIVE"]

    def __call__(self):
        if self.__testing == True:
            return self.__pre_data

        #### Send Signal ####
        self.arduino.write(bytes('YES\n','utf-8'))

        #### Recieve Data ####
        receive_data = self.arduino.readline().decode("utf-8").rstrip()
        self.__data = receive_data.split('\t')
        
        ############ DEBUGING ############
        
        if len(self.__data) != len(self.__pre_data):
            return self.__pre_data

        ########### Processing ###########
        try:
            self.__data = [int(i) for i in self.__data]

            ######## GEAR ########
            id_name_gear = self.__data[4]
            self.__data[4] = self.__name_gear[id_name_gear]

            ######## POWER ########
            id_name_power = self.__data[10]
            self.__data[10] = self.__motor_status[id_name_power]

            ######## CHARGING ########
            id_name_charging = self.__data[6]
            self.__data[6] = self.__name_charging[id_name_charging]

            ######## BRAKING PEDAL ########
            id_name_charging = self.__data[14]
            self.__data[14] = self.__brake[id_name_charging]
            
            ######## DEBUGING ########
            self.__pre_data = self.__data

        except:
            return self.__pre_data
            
        return self.__data
    
    def visualize_speed(self):
        self.__speed[0:-1] = self.__speed[1:]
        self.__speed[-1] = self.__pre_data[1]

        self.__time[0:-1] = self.__time[1:]
        self.__time[-1] = round(time.time() - self.__pre_time,2)

        return self.__speed, self.__time

if __name__ == '__main__':
    canbus = CAN()

    while True:
        data = canbus()

        print(data)


