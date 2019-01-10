from pynq.overlays.base import Overlay
from pynq.lib.video import *
from pynq import MMIO


video_capture = cv2.VideoCapture(0)

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

def draw_rect(face_location):
    (y0, x1, y1, x0) = face_location
    stream.write(0x10, x0) #x0
	stream.write(0x18, y0) #y0
	stream.write(0x20, x1) #x1
	stream.write(0x28, y1) #y1
	stream.write(0x30, 10) #s
    stream.write(0x38, 0x5500FF00) #color

def reset_rect():
    stream.write(0x38, 0x00000000) #color

import cv2
import numpy as np
import face_recognition

print("Starting face detection...")
# Initialize some variables
face_locations = []
face_encodings = []
face_names = []
process_this_frame = True

while True:
    # Grab a single frame of video
    ret, frame = video_capture.read()

    # Resize frame of video to 1/4 size for faster face recognition processing
    small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)

    # Convert the image from BGR color (which OpenCV uses) to RGB color (which face_recognition uses)
    rgb_small_frame = small_frame[:, :, ::-1]

    # Only process every other frame of video to save time
    if process_this_frame:
        # Find all the faces and face encodings in the current frame of video
        face_locations = face_recognition.face_locations(rgb_small_frame)
        #face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

        #face_names = []
        #for face_encoding in face_encodings:
            # See if the face is a match for the known face(s)
            #matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
        #    name = "Unknown"

            # If a match was found in known_face_encodings, just use the first one.
            #if True in matches:
             #   first_match_index = matches.index(True)
             #   name = known_face_names[first_match_index]

        #   face_names.append(name)
        if len(face_locations) > 0:
             draw_rect(face_locations[0])
        else:
            reset_rect()

    process_this_frame = not process_this_frame

    # Hit 'q' on the keyboard to quit!
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release handle to the webcam
print("Ending OpenCV capture...")
video_capture.release()
cv2.destroyAllWindows()

print("Ending HDMI...")
hdmi_out.stop()
hdmi_in.stop()
del hdmi_in, hdmi_out
