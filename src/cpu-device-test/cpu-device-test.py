import traceback

USE_OPENCV=True
DEVICE_COMPOSITION=True
DEVICE_RECTS=2

#box_extra.bit
# -- 0x00 : reserved
# -- 0x04 : reserved
# -- 0x08 : reserved
# -- 0x0c : reserved
# -- 0x10 : Data signal of rect_color
# --        bit 31~0 - rect_color[31:0] (Read/Write)
# -- 0x14 : reserved
# -- 0x18 : Data signal of rect_x0
# --        bit 15~0 - rect_x0[15:0] (Read/Write)
# --        others   - reserved
# -- 0x1c : reserved
# -- 0x20 : Data signal of rect_y0
# --        bit 15~0 - rect_y0[15:0] (Read/Write)
# --        others   - reserved
# -- 0x24 : reserved
# -- 0x28 : Data signal of rect_x1
# --        bit 15~0 - rect_x1[15:0] (Read/Write)
# --        others   - reserved
# -- 0x2c : reserved
# -- 0x30 : Data signal of rect_y1
# --        bit 15~0 - rect_y1[15:0] (Read/Write)
# --        others   - reserved
# -- 0x34 : reserved
# -- 0x38 : Data signal of rect_s
# --        bit 15~0 - rect_s[15:0] (Read/Write)
# --        others   - reserved
# -- 0x3c : reserved
# -- 0x40 : Data signal of idx
# --        bit 7~0 - idx[7:0] (Read/Write)
# --        others  - reserved
# -- 0x44 : reserved
# -- 0x48 : Data signal of write_rect
# --        bit 0  - write_rect[0] (Read/Write)
# --        others - reserved
# -- 0x4c : reserved


if DEVICE_COMPOSITION:
    print("Loading custom overlay...")
    from pynq import Overlay
    base = Overlay("/home/xilinx/jupyter_notebooks/box.bit")
else:
    print("Loading base overlay...")
    from pynq.overlays.base import BaseOverlay
    base = BaseOverlay("base.bit")

from pynq.lib.video import *
from pynq import MMIO

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
        stream = MMIO(0x8000_0000,0x1000)

    def draw_rect(idx, face_location, frame=None):
        (x0, y0, x1, y1) = face_location
        print("Draw rect at ({0}, {1}) ({2}, {3})".format(x0,y0,x1,y1))

        if DEVICE_COMPOSITION:
            stream.write(0x10, int(x0*4)) #x0
            stream.write(0x18, int(y0*4)) #y0
            stream.write(0x20, int(x1*4)) #x1
            stream.write(0x28, int(y1*4)) #y1
            stream.write(0x30, 10) #s
            stream.write(0x38, 0x5500FF00) #color
            stream.write(0x40, int(idx & 0xFF)) #idx
            stream.write(0x48, 1) #write_rect
            stream.write(0x48, 0) #write_rect
        else:
            cv2.rectangle(frame, (x0, y0), (x1, y1), (0, 0, 255), 2)

    def reset_rect(idx):

        if DEVICE_COMPOSITION:
            stream.write(0x38, 0x00000000) #color
            stream.write(0x48, 1) #write_rect
            stream.write(0x48, 0) #write_rect

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
        while True:
            # Grab a single frame of video
            frame = hdmi_in.readframe()

            # Resize frame of video to 1/4 size for faster face recognition processing
            small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)

            # Convert the image from BGR color (which OpenCV uses) to RGB color (which face_recognition uses)
            rgb_small_frame = small_frame[:, :, ::-1]

            # Only process every other frame of video to save time
            if process_this_frame:
                if not USE_OPENCV:
                    # Find all the faces and face encodings in the current frame of video
                    face_locations = face_recognition.face_locations(rgb_small_frame)
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
                    face_cascade = cv2.CascadeClassifier(
                        '/home/xilinx/jupyter_notebooks/base/video/data/'
                        'haarcascade_frontalface_default.xml')
                    eye_cascade = cv2.CascadeClassifier(
                        '/home/xilinx/jupyter_notebooks/base/video/data/'
                        'haarcascade_eye.xml')

                    gray = cv2.cvtColor(rgb_small_frame, cv2.COLOR_BGR2GRAY)
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

                    while idx<DEVICE_RECTS:
                        reset_rect(idx)
                        idx+=1

                    if not DEVICE_COMPOSITION:
                        hdmi_out.writeframe(frame)
                    

            process_this_frame = not process_this_frame

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
