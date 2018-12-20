from pynq.overlays.base import Overlay
from pynq.lib.video import *
from pynq import MMIO

print("Loading overlay...")
base = Overlay("/home/xilinx/jupyter_notebooks/box.bit")
hdmi_in = base.video.hdmi_in
hdmi_out = base.video.hdmi_out
print("Configuring HDMI...")

hdmi_in.configure(PIXEL_RGB)
hdmi_out.configure(hdmi_in.mode, PIXEL_RGB)

print("Starting HDMI...")
hdmi_in.start()
hdmi_out.start()

total_frames = 200
framen = 0

stream = MMIO(0x8000_0000,0x1000)

import cv2
import numpy as np
print("Starting face detection...")
while True:
    frame = hdmi_in.readframe()

    face_cascade = cv2.CascadeClassifier(
        '/home/xilinx/jupyter_notebooks/base/video/data/'
        'haarcascade_frontalface_default.xml')
    eye_cascade = cv2.CascadeClassifier(
        '/home/xilinx/jupyter_notebooks/base/video/data/'
        'haarcascade_eye.xml')

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.3, 5)

	x0 = faces.x
	y0 = faces.y
	x1 = faces.x + faces.w
	y1 = faces.y + faces.h

	stream.write(0x10, x0) #x0
	stream.write(0x18, y0) #y0
	stream.write(0x20, x1) #x1
	stream.write(0x28, y1) #y1
	stream.write(0x30, 10) #s
	stream.write(0x38, 0x5500FF00) #color

    #for (x,y,w,h) in faces:
    #    cv2.rectangle(frame,(x,y),(x+w,y+h),(255,0,0),2)
    #    roi_gray = gray[y:y+h, x:x+w]
    #    roi_color = frame[y:y+h, x:x+w]

    #    eyes = eye_cascade.detectMultiScale(roi_gray)
    #    for (ex,ey,ew,eh) in eyes:
    #        cv2.rectangle(roi_color,(ex,ey),(ex+ew,ey+eh),(0,255,0),2)

    #hdmi_out.writeframe(frame)

    if framen >= total_frames:
        break
    framen += 1


print("Ending HDMI...")
hdmi_out.stop()
hdmi_in.stop()
del hdmi_in, hdmi_out
