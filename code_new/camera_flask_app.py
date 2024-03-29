import base64
from flask import Flask, render_template, Response, request, jsonify
import cv2
import time
import os, sys
import numpy as np


predict=False
switch=False
show_webcam=False
battery_status = False


#Load pretrained face detection model    
net = cv2.dnn.readNetFromCaffe('./saved_model/deploy.prototxt.txt', './saved_model/res10_300x300_ssd_iter_140000.caffemodel')

#instatiate flask app  
app = Flask(__name__, template_folder='./templates')

camera = None

def detect_face(frame):
    global net
    (h, w) = frame.shape[:2]
    blob = cv2.dnn.blobFromImage(cv2.resize(frame, (300, 300)), 1.0,
        (300, 300), (104.0, 177.0, 123.0))   
    net.setInput(blob)
    detections = net.forward()
    confidence = detections[0, 0, 0, 2]

    if confidence < 0.5:            
            return frame           

    box = detections[0, 0, 0, 3:7] * np.array([w, h, w, h])
    (startX, startY, endX, endY) = box.astype("int")
    cv2.rectangle(frame, (startX, startY), (endX, endY), (255, 0, 0), 2)
    return frame
 

def gen_frames():  # generate frame by frame from camera
    if show_webcam: 
        while True:
            success, frame = camera.read() 
            if success:
                if(predict):                
                    frame= detect_face(frame)
                try:
                    ret, buffer = cv2.imencode('.jpg', cv2.flip(frame,1))
                    frame = buffer.tobytes()
                    yield (b'--frame\r\n'
                        b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
                except Exception as e:
                    pass
            else:
                pass


def gen_random():
    length = np.random.randint(10, 20)

    a = np.random.randint(0, 100, size=(length))
    b = np.random.randint(0, 100, size=(length))

    return [{"x": int(x), "y":int(y)} for x, y in zip(a, b)]

def gen_random2():
    length = np.random.randint(10, 20)

    a = np.random.randint(0, 100, size=(length))

    return {"data":a.tolist(), "x_label":list(range(length))}

@app.route('/')
def index():
    return render_template('index2.html', data = {'switch': switch})
    
    
@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route("/visualize_lidar", methods = ['POST'])
def visualize_lidar():
    return jsonify({
        'data': gen_random()
    })

@app.route("/visualize_same_lidar", methods = ['POST'])
def visualize_same_lidar():
    return jsonify(
        gen_random2()
    )
@app.route('/predict_server',methods=['POST','GET'])
def predict_server():
    global predict
    if show_webcam:
        predict = not predict
    return jsonify({"predict": predict})

@app.route('/predict_client', methods = ["POST"])
def predict_client():
    # try:
    data = request.get_json()
    _imageBase64 = data["imageBase64"]
    if(_imageBase64 != None and _imageBase64 != ""):
        arr = np.frombuffer(base64.b64decode(_imageBase64), np.uint8)
        frame = cv2.imdecode(arr, 1)
        if data["predict"]:
            frame = detect_face(frame)
            H, W = frame.shape[:2]
            print(frame.shape)
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
    else:
        camera.release()
        cv2.destroyAllWindows()
        predict = False

        
    show_webcam = not show_webcam
    return jsonify({"show_webcam": show_webcam})

@app.route('/get_details', methods = ["POST"])
def get_details():
    gen = np.random.randint
    details = {
        "Vehicle":{
            "Charge": "OFF",
            "Gear": "N",
            "Odermeter": gen(0, 100),
            "Speed": gen(0, 100)
        },
        "Motor":{
            "Status": "OFF",
            "Temperature": gen(0, 100),
            "Speed": gen(0, 100),
            "Required": gen(0, 100), #Required current
        },
        "Battery":{
            "Status": "ON" if battery_status else "OFF",
            "SOC": gen(0, 100),
            "Temperature": gen(0, 100),
            "Voltage": gen(0, 100)
        }
    }
    return jsonify(details)



@app.route('/get_logo')
def get_logo():
    if battery_status:
        frame = cv2.imread("..\inference\images\charging.png")
    else:
        frame = cv2.imread("..\inference\images\Twizy.png")

    print(frame.shape)
    ret, buffer = cv2.imencode('.png', frame)
    frame = buffer.tobytes()
    res =  (b'--frame\r\n'
        b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
    return Response(res, mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5002)

if camera is not None: 
    camera.release()
    cv2.destroyAllWindows()     