import cv2
import numpy as np
import logging
from time import time, sleep

from trt_inference.cerberus_trt import CerberusInference
from trt_inference.cls import WTR_CLS, SN_CLS, TD_CLS, DET_CLS_IND
from postproc import get_clusters, fast_clustering

from profiler import Profiler

logging.basicConfig(format='%(asctime)s [%(levelname)8s] %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
LOGGER = logging.getLogger(__name__)
LOGGER.setLevel(logging.DEBUG)
Profiler.set_warmup(25)

cls_col = [(153, 255, 102), (255, 255, 255), (0, 255, 255), (52, 255, 52), (51, 153, 51),
           (0, 255, 0), (153, 51, 51), (0, 0, 255), (255, 0, 0)]

lancol = [(0, 255, 255), (255, 255, 255), (255, 150, 50), (0, 0, 255),
          (102, 0, 102), (10, 255, 0), (255, 255, 0), (0, 153, 255)]

class Multitasking:
    def __init__(self, model_file):
        # Classes
        self.__wtr = {v: k for k, v in WTR_CLS.items()}
        self.__scn = {v: k for k, v in SN_CLS.items()}
        self.__td = {v: k for k, v in TD_CLS.items()}

        self.model = CerberusInference(model_file)

        self.__times = []

    def __call__(self, frame, infer_only=False):

        with Profiler('acquire'):
            frame = cv2.resize(frame, (640, 360))
            frame = frame[20:340, :, :]

        try:
            # t = time()

            with Profiler('inference_all'):
                preds = self.model(frame, raw=infer_only)

            if not infer_only:
                det_out, lane_out, scn_out = preds
                boxes = det_out
                lanes, lanes_cls, lanes_votes = lane_out

                # Classification results
                w_cls = self.__wtr[scn_out[0].item()]
                s_cls = self.__scn[scn_out[1].item()]
                td_cls = self.__td[scn_out[2].item()]
                
                # Lane clustering
                with Profiler('lane_clustering'):
                    lane_clusters = fast_clustering(lanes, lanes_cls, lanes_votes)

                # Draw keypoints
                with Profiler('lane_drawing'):
                    for cla, cls_clusters in enumerate(lane_clusters):
                        for cl in cls_clusters:

                            col = lancol[cla]
                            if cl.shape[0] < 5:
                                continue

                            x = cl[:, 0]
                            y = cl[:, 1]

                            # calculate polynomial
                            try:
                                z = np.polyfit(x, y, 2)
                                f = np.poly1d(z)
                            except ValueError:
                                continue

                            # calculate new x's and y's
                            x_new = np.linspace(min(x), max(x), len(x) * 2)
                            y_new = f(x_new)

                            for cx, cy in zip(x_new, y_new):
                                frame = cv2.circle(frame, (int(cx), int(cy)), 1, col, thickness=2, )

                # Draw boxes
                with Profiler('det_drawing'):
                    for b in boxes:
                        cls = DET_CLS_IND[int(b[5])].split(" ")[-1]
                        tl = (int(b[2]), int(b[3]))
                        br = (int(b[0]), int(b[1]))

                        color = (0, 255, 0) if b[6] < 0.5 else (0,0,255)
                        cv2.rectangle(frame, tl, br, color, 2)

                        (text_width, text_height), _ = cv2.getTextSize(cls, cv2.FONT_HERSHEY_DUPLEX, 0.3, 1)
                        cv2.rectangle(frame, br, (br[0] + text_width - 1, br[1] + text_height - 1),
                                    color, cv2.FILLED)
                        cv2.putText(frame, cls, (br[0], br[1] + text_height - 1), cv2.FONT_HERSHEY_DUPLEX,
                                    0.3, 0, 1, cv2.LINE_AA)

                # Add text
                with Profiler('cls_drawing'):
                    text = f"WEATHER: {w_cls}   SCENE: {s_cls}   DAYTIME: {td_cls}"
                    frame = cv2.rectangle(frame, (10, 5), (550, 25), (0, 0, 0), -1)
                    frame = cv2.putText(frame, text, (15, 20), cv2.FONT_HERSHEY_DUPLEX, 0.5,
                                        (255,255,255), 1, cv2.LINE_AA, False)

                # cv2.imshow("result", frame)
            
            # =================Timing Stats================= #
            # dt = time() - t
            # self.__times.append(dt)

            # print(f"{'AVERAGE TIME:':<37}{np.array(self.__times[10:]).mean()*1000:>6.3f} ms")
            # print(f"{'AVERAGE FPS:':<37}{(1/np.array(self.__times[10:]).mean()):>6.3f} FPS")
            
        except:
            print('error')
            pass
        
        return frame

if __name__ == '__main__':
    # this won't be run when imported
    detections = Multitasking('weights/mobilenetv2_bifpn_sim.onnx')

    frame = cv2.imread('./docs/test.png')

    frame = detections(frame)
    cv2.imshow("result", frame)

    cv2.waitKey(0)
    cv2.destroyAllWindows()
