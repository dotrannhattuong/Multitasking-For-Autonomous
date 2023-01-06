import serial
import matplotlib.pyplot as plt
import math
import numpy as np
import time

class Lidar:
    def __init__(self):
        self.__testing = False
        try:
            board = '/dev/ttyUSB0'
            # board = 'COM8'
            baud = 500000
            self.ser = serial.Serial(board, baud, timeout=0.01)
        except:
            self.__testing = True

        print(f"Testing LIDAR: {self.__testing}")

    def __call__(self):
        if self.__testing == True:
            return [0], [0]
        
        try:
            start_time = time.time()
            #### Send Data ####
            msg = b"sRN LMDscandata"
            msg = b"\x02" + msg + b"\x03"
            self.ser.write(msg)

            #### Recieve Data ####
            recieve_data = self.ser.readline().decode('utf-8')
            recieve_data = recieve_data.split(' ') # Convert String to List

            #### Get Start Angle ####
            res = len(recieve_data) - 1 - recieve_data[::-1].index('1388')
            start_angle = recieve_data[res - 1]
            start_angle = int('0x' + start_angle, 16)//10000 # Convert Hex to Dec

            #### Get Number of Data ####
            number_of_object = int("0x" + recieve_data[res+1], 16)

            #### Get Distance of Objects (mm) ####
            distance_of_odjects = recieve_data[res+2:-6]
            distance_of_odjects = [int('0x' + object, 16) for object in distance_of_odjects]

            #### Get Min-Max Distance ####
            # Min_Distance = min(distance_of_odjects)
            # Max_Distance = max(distance_of_odjects)

            # print(f"Min_Distance: {Min_Distance}")
            # print(f"Max_Distance: {Max_Distance}")

            #### Get Dictionary Angle:Distance ####
            list_angle = [angle * 3.14 / 180 for angle in np.arange(start_angle, start_angle + number_of_object/2, 0.5)]

            #### Convert To Cartesian coordinate system ####
            x = [distance * math.cos(angle_rad) for angle_rad, distance in zip(list_angle, distance_of_odjects)]
            y = [distance * math.sin(angle_rad) for angle_rad, distance in zip(list_angle, distance_of_odjects)]

            # angle_distance = dict(zip(list_angle, distance_of_odjects))
            # print(angle_distance)

            print(f"FPS: {1/(time.time() - start_time)}")
            
        except:
            return [0], [0]

        return x, y

if __name__ == '__main__':
    distance_meter = Lidar()

    while True:
        x, y = distance_meter()

        #### Visualize ####
        plt.title('LIDAR')
        plt.scatter(x, y)
        plt.pause(0.005)
        plt.clf()