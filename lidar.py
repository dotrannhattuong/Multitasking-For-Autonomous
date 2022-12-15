import serial
import matplotlib.pyplot as plt
import math

board = '/dev/ttyUSB0'
baud = 115200
ser = serial.Serial(board, baud, timeout=0.05)

def distance( x1, y1, x2, y2):
    return (math.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2))

# def lidar(out_q):
while True:
    msg = b"sRN LMDscandata"
    msg = b"\x02" + msg + b"\x03"
    ser.write(msg)
    str2 = ser.readline()
    print(str2)
    # i = 0
    # str = ""
    # data = 0
    # distance = []
    # if str2.rfind("1388") != -1:
    #     str1 = str2[slice(str2.rfind("1388") + 5, len(str2), 1)]
    #     data = int("0x" + str1[slice(0, 2, 1)], 16)
    # if data > 0:
    #     str1 = str1[slice(2, len(str1), 1)]
    # while i < data:
    #     dodai = None
    #     str1 = str1[slice(1, len(str1), 1)]
    #     dodai = str1.find(" ")
    #     str = str1[slice(0, dodai, 1)]
    #     hexconvert = "0x" + str
    #     hex = int(hexconvert, 16)
    #     distance.append(hex)
    #     str1 = str1[slice(dodai, len(str1), 1)]
    #     i = i + 1
    # i = 0
    # min=0
    # index=None
    # while i+10 < data-10:
    #     degree = 55 + 0.5 * i
    #     if min<distance[i]:
    #         min=distance[i]
    #         index=i
    #     rad = degree * 3.14 / 180
    #     x = distance[i] * math.cos(rad)
    #     y = distance[i] * math.sin(rad)
    #     # plt.plot(x, y, 'bo')
    #     # fig.show()
    #     # fig.canvas.draw()
        
    #     # plt.clf()
    #     i = i + 1
    # # location = 55 + 0.5 * index
    # # out_q.put(location)
    # print(index)