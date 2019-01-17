import traceback

USE_OPENCV=True
DEVICE_COMPOSITION=True
DEVICE_RECTS=8

if DEVICE_COMPOSITION:
    print("Loading custom overlay...")
    from pynq import Overlay
    base = Overlay("/home/xilinx/bitstream/box_packed_hsv.bit")
else:
    print("Loading base overlay...")
    from pynq.overlays.base import BaseOverlay
    base = BaseOverlay("base.bit")

from pynq.lib.video import *
from pynq import MMIO
import random
import math
import time
import struct

hdmi_in = base.video.hdmi_in
hdmi_out = base.video.hdmi_out
print("Configuring HDMI...")

try:
    hdmi_in.configure(PIXEL_RGBA)
    hdmi_out.configure(hdmi_in.mode, PIXEL_RGBA)

    print("Starting HDMI...")
    hdmi_in.start()
    hdmi_out.start()

    if DEVICE_COMPOSITION:
        hdmi_in.tie(hdmi_out)

    total_frames = 200
    framen = 0

    if DEVICE_COMPOSITION:
        stream = MMIO(0x7FFF_8000, 0x7FFF)

    def draw_rect(idx, face_location, frame=None):
        (x0, y0, x1, y1) = face_location
        print("Draw rect {4} at ({0}, {1}) ({2}, {3})".format(x0,y0,x1,y1,idx))

        if DEVICE_COMPOSITION:
            data = struct.pack(">HHHHHHHH",
                int(idx & 0xFFFF), # index, 16-bits (just for padding)
                1, # ON signal, 16-bit
                (random.randint(0x40,0xFF) << 8) | random.randint(0,0xFF), # alpha and hue, 16-bit
                int(x0*4), #x0, 16-bit
                int(y0*4), #y0, 16-bit
                int(x1*4), #x1, 16-bit
                int(y1*4), #y1, 16-bit
                math.ceil((x1-x0)/4) #stroke, 16-bit
            )
            stream.write(0x10,data)
        else:
            cv2.rectangle(frame, (x0*4, y0*4), (x1*4, y1*4), (random.randint(0,0xFF), random.randint(0,0xFF), random.randint(0,0xFF)), math.ceil((x1-x0)/4))

    def reset_rect(idx):
        print("Remove rect {0}".format(idx))
        if DEVICE_COMPOSITION:
            data = struct.pack("HH",
                int(idx & 0xFFFF), # 16-bits (just for padding)
                0
            )
            stream.write(0x10,data)

    import cv2
    import numpy as np
    if not USE_OPENCV:
        import face_recognition

    print("Starting face detection...")
    # Initialize some variables
    face_locations = []
    face_encodings = []
    face_names = []
    process_this_frame = True
    try:
        face_cascade = cv2.CascadeClassifier(
                        '/home/xilinx/jupyter_notebooks/base/video/data/'
                        'haarcascade_frontalface_default.xml')

        start_frametime = 0.0
        end_frametime = 0.0
        while True:
            start_frametime = time.perf_counter()
            # Grab a single frame of video
            frame = hdmi_in.readframe()

            # Resize frame of video to 1/4 size for faster face recognition processing
            small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)

            # Convert the image from BGR color (which OpenCV uses) to RGB color (which face_recognition uses)
            # rgb_small_frame = small_frame[:, :, ::-1]
            if not USE_OPENCV:
                # Find all the faces and face encodings in the current frame of video
                face_locations = face_recognition.face_locations(small_frame)
                #face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

                #   face_names.append(name)
                if len(face_locations) >= 2:
                    draw_rect(0,face_locations[0], frame=frame)
                    draw_rect(1,face_locations[1], frame=frame)
                elif len(face_locations) >= 1:
                    draw_rect(0,face_locations[0], frame=frame)
                    reset_rect(1)
                else:
                    reset_rect(0)
                    reset_rect(1)
            else:

                gray = cv2.cvtColor(small_frame, cv2.COLOR_RGB2GRAY)
                faces = face_cascade.detectMultiScale(gray, 1.3, 5)

                print("Found {} faces.".format(len(faces)))
                idx = 0
                for (x, y, w, h) in faces:
                    if idx >= DEVICE_RECTS:
                        break
                    x0 = x
                    y0 = y
                    x1 = x + w
                    y1 = y + h
                    draw_rect(idx,(x0, y0, x1, y1), frame=frame)
                    idx+=1

                while idx < DEVICE_RECTS:
                    reset_rect(idx)
                    idx+=1

                if not DEVICE_COMPOSITION:
                    hdmi_out.writeframe(frame)
                    
            end_frametime = time.perf_counter()
            print("Frame time: {0:.3f}; FPS: {1:.3f}".format(end_frametime-start_frametime,1/(end_frametime-start_frametime)))
            # Hit 'q' on the keyboard to quit!
            if cv2.waitKey(1) & 0xFF == ord('q'):
                print("Quitting...")
                break
    except KeyboardInterrupt:
        print("Got Ctrl+C. Quitting...")
except Exception:
    traceback.print_exc()
    print("Got Exception. Quitting...")

print("Ending HDMI...")
hdmi_out.stop()
hdmi_in.stop()
del hdmi_in, hdmi_out
