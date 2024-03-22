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

def detect_and_decode_qr():
    # Initialize the webcam
    # Adding cv2.CAP_DSHOW increased the init speed
    print("Connecting to camera: 0")
    # cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
    cap = cv2.VideoCapture(0)
    print("Connected")
    cap.set(3, 1920)
    cap.set(4, 1080)
    
    try:
        # Loop to continuously to get frames from the webcam
        while True:
            # Capture a single frame
            ret, frame = cap.read()

            # If frame is read correctly ret is True
            if not ret:
                continue  # Skip the rest of the code and try again
            
            frame = cv2.flip(frame, 1) 
            cv2.imshow('Video Feed', frame)
            cv2.waitKey(1)

            # Try to decode any QR codes in the image
            qr_codes = decode(frame)

            # If at least one QR code is detected
            if qr_codes:
                # Decode the QR code and return the data
                cv2.destroyAllWindows() 
                data = qr_codes[0].data.decode('utf-8')
                uuid = extract_uuid(data)
                print(f"Found: {uuid}, {data}")
                return uuid, data
    except Exception as e:
        print("Trying again: ", e, flush=True)
    finally:
        # When everything done, release the capture
        cap.release()

if __name__ == '__main__':
    # Example usage:
    uuid, qr_data = detect_and_decode_qr()
    print(f"Decoded QR code contents: {uuid, qr_data}")


# Now that the QR code has been read, parse the ID from the url and list possible actions and wait for input.

# Create radio in the database.