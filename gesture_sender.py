import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import time
import urllib.request
import os

# --- Download model if not present ---
model_path = "hand_landmarker.task"
if not os.path.exists(model_path):
    print("Downloading hand landmark model...")
    urllib.request.urlretrieve(
        "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task",
        model_path,
    )
    print("Model downloaded!")

# --- Socket setup ---
HOST = "127.0.0.1"
PORT = 65432

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))
print("Connected to C++ server")

# --- MediaPipe setup ---
latest_gesture = "NONE"
latest_x = 0.0
letest_y = 0.0


def get_finger_states(landmarks):
    fingers = []
    # Thumb
    fingers.append(landmarks[4].x < landmarks[3].x)
    # Other fingers
    for tip in [8, 12, 16, 20]:
        fingers.append(landmarks[tip].y < landmarks[tip - 2].y)
    return fingers


def recognise_gesture(fingers):
    thumb, index, middle, ring, pinky = fingers
    if not any(fingers):
        return "FIST"
    if all(fingers):
        return "OPEN_HAND"
    if index and not middle and not ring and not pinky:
        return "POINTING"
    if index and middle and not ring and not pinky:
        return "PEACE"
    if thumb and not index and not middle and not ring and not pinky:
        return "THUMBS_UP"
    return "UNKNOWN"


def get_index_tip_position(landmarks):
    """Returns normalised x,y of index fingertip (landmark 8)"""
    tip = landmarks[8]
    return tip.x, tip.y


def result_callback(result, output_image, timestamp_ms):
    global latest_gesture, latest_x, latest_y
    if result.hand_landmarks:
        landmarks = result.hand_landmarks[0]
        fingers = get_finger_states(landmarks)
        latest_gesture = recognise_gesture(fingers)
        latest_x, latest_y = get_index_tip_position(landmarks)
    else:
        latest_gesture = "NONE"
        latest_x, latest_y = 0.0, 0.0


# --- Build detector ---
base_options = python.BaseOptions(model_asset_path=model_path)
options = vision.HandLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.LIVE_STREAM,
    num_hands=1,
    min_hand_detection_confidence=0.7,
    result_callback=result_callback,
)
detector = vision.HandLandmarker.create_from_options(options)

# --- Camera setup ---
cap = cv2.VideoCapture(0)
last_sent = 0
COOLDOWN = 0.8

timestamp = 0

while True:
    success, frame = cap.read()
    if not success:
        break

    frame = cv2.flip(frame, 1)
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
    detector.detect_async(mp_image, timestamp)
    timestamp += 1

    # Display gesture on screen
    cv2.putText(
        frame, latest_gesture, (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 0), 2
    )
    cv2.imshow("Gesture Sender", frame)

    # Send gesture with cooldown
    now = time.time()
    if latest_gesture not in ("NONE", "UNKNOWN") and now - last_sent > COOLDOWN:
        try:
            msg = f"{latest_gesture}:{latest_x:.4f}:{latest_y:.4f}"
            sock.sendall((latest_gesture + "\n").encode())
            print(f"Sent: {latest_gesture}")
            last_sent = now
        except:
            print("Lost connection to C++ server")
            break

    if cv2.waitKey(1) == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
sock.close()
