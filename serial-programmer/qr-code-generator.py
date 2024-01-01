import qrcode
from PIL import Image, ImageDraw
from uuid import uuid1
import io

# Settings for the paper
PAPER_WIDTH = 8.5
PAPER_HEIGHT = 11
DPI = 300  # For printing, 300 DPI is standard

# Convert inches to pixels
px_width = int(PAPER_WIDTH * DPI)
px_height = int(PAPER_HEIGHT * DPI)

# Settings for the QR codes grid
COLUMNS = 6
ROWS = 8
# Margin between QR codes (in pixels)
MARGIN = 1

# Calculate QR code size (subtract margins and divide by the number of QR codes)
qr_size = min(
    (px_width - (COLUMNS + 1) * MARGIN) // COLUMNS,
    (px_height - (ROWS + 1) * MARGIN) // ROWS
)

# Create a blank canvas
canvas = Image.new('RGB', (px_width, px_height), 'white')
draw = ImageDraw.Draw(canvas)

for row in range(ROWS):
    for col in range(COLUMNS):
        # Generate a unique UUIDv1
        unique_id = str(uuid1())
        url = f"http://example.com/{unique_id}"

        # Generate QR code
        qr = qrcode.QRCode(
            version=1,
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=10,
            border=4,
        )
        qr.add_data(url)
        qr.make(fit=True)

        # Generate an image from the QR Code instance
        img_qr = qr.make_image(fill='black', back_color='white').convert('RGB')

        # Calculate position of top-left corner of this QR code
        x = MARGIN + (MARGIN + qr_size) * col
        y = MARGIN + (MARGIN + qr_size) * row

        # Resize to fit the grid cell if necessary
        img_qr.thumbnail((qr_size, qr_size))

        # Paste the QR code onto the canvas
        canvas.paste(img_qr, (x, y))

# Save the result to a file
output_filename = 'qr_grid.pdf'
canvas.save(output_filename, "PDF", resolution=DPI)

print(f"QR codes grid saved to {output_filename}")

import subprocess


subprocess.run(['lp', output_filename])