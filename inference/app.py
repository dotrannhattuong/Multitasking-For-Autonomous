import base64
from flask import Flask, render_template, Response, request, jsonify
import cv2
import time
import numpy as np
from detection import Multitasking
from lidar import Lidar
from my_can import CAN

#### Embedded System ####
distance_meter = Lidar()
canbus = CAN()

predict=False
switch=False
show_webcam=False
battery_status = False
camera = None

# Load model
model = Multitasking('weights/mobilenetv2_bifpn_sim.onnx', split_frames=1)     

# instatiate flask app  
app = Flask(__name__, template_folder='./templates')

def gen_frames():  # generate frame by frame from camera
    if show_webcam: 
        while True:
            success, frame = camera.read()    
            if success:
                if predict:
                    frame = model(frame)

                try:
                    ret, buffer = cv2.imencode('.jpg', frame)
                    frame = buffer.tobytes()
                    yield (b'--frame\r\n'
                        b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
                except Exception as e:
                    pass
                    
            else:
                pass

def lidar_visualize():
    x, y = distance_meter()
    return [{"x": int(a), "y":int(b)} for a, b in zip(x, y)]

def speed_visualize():
    spd, real_time = canbus.visualize_speed()
    return {"data":spd.tolist(), "x_label":real_time.tolist()} # list(range(length))

@app.route('/')
def index():
    return render_template('index.html', data = {'switch': switch})
    
@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route("/visualize_lidar", methods = ['POST'])
def visualize_lidar():
    return jsonify({
        'data': lidar_visualize()
    })

@app.route("/visualize_speed", methods = ['POST'])
def visualize_speed():
    return jsonify(
        speed_visualize()
    )

@app.route('/predict_server',methods=['POST','GET'])
def predict_server():
    global predict
    if show_webcam:
        predict = not predict
    return jsonify({"predict": predict})

@app.route('/predict_client', methods = ["POST"])
def predict_client():
    data = request.get_json()
    _imageBase64 = data["imageBase64"]
    if(_imageBase64 != None and _imageBase64 != ""):
        arr = np.frombuffer(base64.b64decode(_imageBase64), np.uint8)
        frame = cv2.imdecode(arr, 1)
        if data["predict"]:
            frame = model(frame)
            H, W = frame.shape[:2]
    
            retval, buffer = cv2.imencode('.jpg', frame)
            strBase64 = base64.b64encode(buffer)
            return jsonify({
                'imageBase64': strBase64.decode("ascii"),
                'w': W,
                'h': H
            })
        else:
            return jsonify({
                'imageBase64': "",
            })

@app.route('/get_webcam',methods=['POST'])
def get_webcam():
    global switch, camera, show_webcam, predict
    if not show_webcam:
        camera = cv2.VideoCapture(0)
        camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 360)
        
    else:
        camera.release()
        camera = None
        cv2.destroyAllWindows()
        predict = False

        
    show_webcam = not show_webcam
    return jsonify({"show_webcam": show_webcam})

@app.route('/get_details', methods = ["POST"])
def get_details():
    global battery_status
    gen = np.random.randint
    data = canbus()

    battery_status = True if data[6] == 'CHARGING' else False
    # battery_status = True if gen(0, 1000) % 2 ==0 else False

    details = {
        "Vehicle":{
            "Status": data[6],
            "Gear": data[4],
            "Odermeter": data[2],
            "Speed": data[1],
        },
        "Motor":{
            "Status": data[10],
            "Temperature": data[3],
            "Speed": data[0],
            "Required": data[12]
        },
        "Battery":{
            "Charge": "ON" if battery_status else "OFF",
            "SOC": data[7],
            "Temperature": data[9], 
            "Voltage": data[5] 
        }
    }
    return jsonify(details)

def gen_logo():
    while True:
        if battery_status:
            frame = cv2.imread("inference/images/charging2.png")
        else:
            frame = cv2.imread("inference/images/Twizy.png")

        ret, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()
        yield (b'--frame\r\n'
        b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/get_logo')
def get_logo():
    # print("GET LOGO", battery_status)
    # if battery_status:
    #     frame = cv2.imread("inference/images/charging.png")
    # else:
    #     frame = cv2.imread("inference/images/Twizy.png")

    # ret, buffer = cv2.imencode('.jpg', frame)
    # frame = buffer.tobytes()
    # res =  (b'--frame\r\n'
    #     b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
    # return Response(res, mimetype='multipart/x-mixed-replace; boundary=frame')
    return Response(gen_logo(), mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000)

if camera is not None:   
    camera.release()
    cv2.destroyAllWindows()