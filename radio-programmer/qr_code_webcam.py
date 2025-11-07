"""QR code detection via webcam.

Linux-friendly camera initialization with auto-detection of a working
video device using the V4L2 backend.
"""

#  pip install opencv-python pyzbar
import cv2
import re
from pyzbar.pyzbar import decode


def extract_uuid(s):
    """
    Extracts a UUID from a given string.
    Returns the UUID as a string if found, otherwise returns None.
    """
    uuid_regex = r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"
    match = re.search(uuid_regex, s)
    return match.group(0) if match else None


def _open_first_working_camera(preferred_indices=(0, 1, 2, 3)):
    """
    Try common camera indices with the V4L2 backend and return an opened
    VideoCapture and the index if successful.
    """
    for idx in preferred_indices:
        print(f"Trying camera index: {idx}")
        cap = cv2.VideoCapture(idx, cv2.CAP_V4L2)
        if cap is None or not cap.isOpened():
            if cap is not None:
                cap.release()
            continue
        # Validate we can read at least one frame
        ok, _ = cap.read()
        if ok:
            print(f"Connected to camera index: {idx}")
            return cap, idx
        cap.release()
    raise RuntimeError("No working camera found (tried indices 0-3)")


def detect_and_decode_qr():
    # Initialize the webcam (auto-detect a working device)
    cap, cam_idx = _open_first_working_camera()

    # Set desired resolution (may be ignored by some drivers)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                continue

            frame = cv2.flip(frame, 1)
            cv2.imshow(f"Video Feed (camera {cam_idx})", frame)
            cv2.waitKey(1)

            qr_codes = decode(frame)
            if qr_codes:
                data = qr_codes[0].data.decode('utf-8')
                uuid = extract_uuid(data)
                print(f"Found: {uuid}, {data}")
                cv2.destroyAllWindows()
                return uuid, data
    except Exception as e:
        print("Trying again: ", e, flush=True)
    finally:
        cap.release()
        cv2.destroyAllWindows()

if __name__ == '__main__':
    # Example usage:
    uuid, qr_data = detect_and_decode_qr()
    print(f"Decoded QR code contents: {uuid, qr_data}")


# Now that the QR code has been read, parse the ID from the url and list possible actions and wait for input.

# Create radio in the database.