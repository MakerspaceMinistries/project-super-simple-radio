import cv2
from pyzbar.pyzbar import decode

def detect_and_decode_qr():
    # Initialize the webcam
    cap = cv2.VideoCapture(0)

    cap.set(3, 1920)
    cap.set(4, 1080)
    
    try:
        # Loop to continuously get frames from the webcam
        while True:
            # Capture a single frame
            ret, frame = cap.read()

            # If frame is read correctly ret is True
            if not ret:
                continue  # Skip the rest of the code and try again
            
            cv2.imshow('Video Feed', frame)
            cv2.waitKey(1)

            # Try to decode any QR codes in the image
            qr_codes = decode(frame)

            # If at least one QR code is detected
            if qr_codes:
                # Decode the QR code and return the data
                return qr_codes[0].data.decode('utf-8')
    finally:
        # When everything done, release the capture
        cap.release()

# Example usage:
qr_data = detect_and_decode_qr()
print(f"Decoded QR code contents: {qr_data}")


# Now that the QR code has been read, parse the ID from the url and list possible actions and wait for input.

# Create radio in the database.