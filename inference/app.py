import base64
from flask import Flask, render_template, Response, request, jsonify
import cv2
import datetime, time
import os, sys
import numpy as np
from detection import Multitasking

predict=False
switch=False
show=False

# Load model
model = Multitasking('weights/mobilenetv2_bifpn_sim.onnx', split_frames=2)     

#instatiate flask app  
app = Flask(__name__, template_folder='./templates')

def gen_frames():  # generate frame by frame from camera
    if show: 
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

@app.route('/')
def index():
    return render_template('index.html', data = {'switch': switch})
    
@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

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
                'imageBase64': "1",
            })

@app.route('/requests',methods=['POST','GET'])
def tasks():
    global switch, camera, show
    if request.method == 'POST':
        if  request.form.get('predict') in ['Predict', 'Predicting...']:
            global predict
            if show:
                predict = not predict

            if predict:
                time.sleep(1) 
  
        if  request.form.get('status') in ['Start', 'Stop']:
            if request.form.get('status') == "Start":
                show = True
                switch=True
                camera = cv2.VideoCapture(0)
            
            else:
                show=False
                switch=False
                predict = False
                camera.release()
                cv2.destroyAllWindows()                          
                 
    elif request.method=='GET':
        return render_template('index.html', data = {'switch': switch})
    data = {
        'switch': switch,
        'predict': predict
    }
    return render_template('index.html', data = data)

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000)
    
camera.release()
cv2.destroyAllWindows()