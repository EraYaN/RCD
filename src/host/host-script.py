import traceback

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

    # Function that actaully set the rectangle regsiters.
    def draw_rect(idx, face_location, frame=None):
        (x0, y0, x1, y1) = face_location
        print("Draw rect {4} at ({0}, {1}) ({2}, {3})".format(x0,y0,x1,y1,idx))

        # -- 0x00 : reserved
        # -- 0x04 : reserved
        # -- 0x08 : reserved
        # -- 0x0c : reserved
        # -- 0x10 : Data signal of idx
        # --        bit 15~0 - idx[15:0] (Read/Write)
        # --        others   - reserved
        # -- 0x14 : reserved
        # -- 0x18 : Data signal of rect_in_V_2
        # --        bit 31~0 - rect_in_V_2[31:0] (Read/Write)
        # -- 0x1c : Data signal of rect_in_V_2
        # --        bit 31~0 - rect_in_V_2[63:32] (Read/Write)
        # -- 0x20 : Data signal of rect_in_V_2
        # --        bit 31~0 - rect_in_V_2[95:64] (Read/Write)
        # -- 0x24 : Data signal of rect_in_V_2
        # --        bit 15~0 - rect_in_V_2[111:96] (Read/Write)
        # --        others   - reserved
        # -- 0x28 : reserved
        # -- (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
        
        # rect.is_on = (rect_in(96,96) != 0);
        # rect.color = ah2argb(rect_in(95,80));
        # rect.x0 = rect_in(79,64);
        # rect.y0 = rect_in(63,48);
        # rect.x1 = rect_in(47,32);
        # rect.y1 = rect_in(31,16);
        # rect.s = rect_in(15,0);
        if DEVICE_COMPOSITION:            
            data = struct.pack("<HHIHHHHHHHH", # Big-endian
                int(idx & 0xFFFF), # index, 16-bits (just for padding)
                0, # 16-bit padding
                0, # 32-bit reserved space,
                math.ceil((x1-x0)/4), # stroke, 16-bit
                int(y1*4), # y1, 16-bit
                int(x1*4), # x1, 16-bit
                int(y0*4), # y0, 16-bit
                int(x0*4), # x0, 16-bit
                (random.randint(0,0xFF) << 8) | random.randint(0x40,0xFF), # hue & alpha (alpha between 0x40 and 0xFF and hue between 0x0 and 0xFF), 16-bit                
                0x8000, # ON signal, 16-bit (single bit checked)
                0 # 16-bit padding
            )
            stream.write(0x10,data)
        else:
            cv2.rectangle(frame, (x0*4, y0*4), (x1*4, y1*4), (random.randint(0,0xFF), random.randint(0,0xFF), random.randint(0,0xFF)), math.ceil((x1-x0)/4))

    def reset_rect(idx):
        print("Remove rect {0}".format(idx))
        if DEVICE_COMPOSITION:
            data = struct.pack("<HHIHHHHHHHH", # Big-endian
                int(idx & 0xFFFF), # index, 16-bits (just for padding)
                0, # 16-bit padding
                0, # 32-bit reserved space,
                0, # stroke, 16-bit
                0, # y1, 16-bit
                0, # x1, 16-bit
                0, # y0, 16-bit
                0, # x0, 16-bit
                0, # hue & alpha, 16-bit                
                0, # OF signal, 16-bit (single bit checked)
                0 # 16-bit padding
            )
            stream.write(0x10,data)

    import cv2
    import numpy as np

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
           
            # Convert to grayscale and recognize all faces.
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
    except KeyboardInterrupt:
        print("Got Ctrl+C. Quitting...")
except Exception:
    traceback.print_exc()
    print("Got Exception. Quitting...")

print("Ending HDMI...")
hdmi_out.stop()
hdmi_in.stop()
del hdmi_in, hdmi_out
