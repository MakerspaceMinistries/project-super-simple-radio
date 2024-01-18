'''

This needs refactored and a class object should probably be used.


Examples:

Auto detect the serial port and write firmware.
python .\serial_programmer.py write_firmware -d --firmware-version v1.0.0-beta.2

'''

import serial
import json
import argparse
import requests
from time import sleep
from qr_code_webcam import detect_and_decode_qr
import esptool
from libs.detect_serial_ports import serial_ports

parser = argparse.ArgumentParser(description='Command line radio configuration.')

parser.add_argument('action', help='Action to be performed.', choices=['create', 'update', 'write_firmware'])

parser.add_argument('-a', '--api-version',  default='v1', type=str, help='Version of the api to use. Default: v1') 
parser.add_argument('-b', '--pcb-version', type=str, help='PCB version.')
parser.add_argument('-c', '--config-endpoint-version', type=str, default='v1.0', help='v1.0')
parser.add_argument('-d', '--auto-detect-serial-port', action='store_true', default=False, help='Attempt to detect a serial port and write to it. UNPLUG OTHER SERIAL DEVICES.')
parser.add_argument('-e', '--has-channel-pot', default=True, type=bool, help='has_channel_pot')
parser.add_argument('-f', '--file', help='Path to the settings file')
parser.add_argument('-H', '--host', type=str, help='Host of the radio configuration server.') 
# parser.add_argument('-i', '--radio-id', type=str, help='ID of the radio. If blank, it will use the webcam to scan for a QR code that contains a URL that ends in a UUID and it will create a radio in the database.')
parser.add_argument('-l', '--label', type=str, help='Label for the radio.')
parser.add_argument('-n', '--network', type=int, help='network_id of the network to which the radio belongs.')
parser.add_argument('-p', '--password', type=str, help='Password for the configuration server.')
parser.add_argument('-r', '--remote-config-background-retrieval-interval', default=43200000, type=int, help='remote_config_background_retrieval_interval')
parser.add_argument('-t', '--target', default='/dev/ttyACM0', type=str, help='Target device to write to. Default: /dev/ACM0')
parser.add_argument('-u', '--user', type=str, help='User name for the configuration server.')
parser.add_argument('-V', '--firmware-version', type=str, help='Version used when writing firmware. Ex: v1.0.0')
parser.add_argument('-w', '--wifi', action='store_true', help='Configure the radio to connect to the default WiFi Network.')

args = parser.parse_args()

def read_config(port):
    with serial.Serial(port, 115200, timeout=2) as ser:
        while True:
            ser.flush()
            ser.write("{}".encode())
            sleep(0.1)
            data = ser.read_all().decode().strip()
            if data:
                print(data, flush=True)
                break

def create_radio(args):
    radio_id, qr_data = detect_and_decode_qr()
    
    with requests.Session() as session:

        # Construct login URL
        login_url = f"{args.host}/api/{args.api_version}/admins/sessions"
        
        # Login payload
        login_payload = {
            'username': args.user,
            'password': args.password
        }

        # Attempt to log in
        response = session.post(login_url, json=login_payload)

        if response.status_code < 400:
            
            # Construct create radio URL
            create_radio_url = f"{args.host}/api/{args.api_version}/radios"
            
            # Create radio payload
            radio_payload = {
                'radio_id': radio_id,
                'network_id': args.network,
                'label': args.label
            }
            
            # Attempt to create the radio
            create_response = session.post(create_radio_url, json=radio_payload)

            if create_response.status_code > 399:
                error_message = create_response.json()["message"]
                raise Exception(f"Failed to create radio: {error_message}")
            
            return radio_id
        
        else:
            raise Exception("Failed to log in")

def write_to_serial(port, payload):
    # Open the serial port
    with serial.Serial(port, 115200) as ser:
        # Check if the payload is a dictionary
        if isinstance(payload, dict):
            # Convert the dictionary to a JSON string
            payload = json.dumps(payload)
        
        # Write the payload to the serial port
        # The encode() method converts the string to bytes, which is required by the write() method
        print(f"Writing: {payload}")
        ser.write(payload.encode())
        ser.flush()
        sleep(0.1)

def update_radio(args):
    radio_id, qr_data = detect_and_decode_qr()

    write_key_value_to_serial(args.target, "remote_config",  True)
    write_key_value_to_serial(args.target, "radio_id", radio_id)
    write_key_value_to_serial(args.target, "remote_cfg_url",  f"{args.host}/api/{args.api_version}/radios/device_interface/{args.config_endpoint_version}/")
    write_key_value_to_serial(args.target, "has_channel_pot",  args.has_channel_pot)
    write_key_value_to_serial(args.target, "pcb_version", args.pcb_version )
    write_key_value_to_serial(args.target, "remote_config_background_retrieval_interval", args.remote_config_background_retrieval_interval )
    if args.wifi:
        wifi_creds = {"ssid": "radio-setup", "pass": "supersimpleradio"}
        write_to_serial(args.target, wifi_creds)

def write_key_value_to_serial(port, key, value):
    kv = {key:value}
    write_to_serial(port, kv)

def load_settings_file(args):
    with open(args.file, 'r') as file:
        settings = json.load(file)
        for key in settings:
            args.__setattr__(key, settings[key])

if __name__ == '__main__':

    # TODO add ability to generate QR codes
    
    # TODO Do this here, pass it as arg
    # radio_id, qr_data = detect_and_decode_qr()

    if args.file:
        load_settings_file(args)

    if args.action in ['create', 'update']:
         # This will print out the config. It should also verify the config.
        read_config(args.target)

    if args.action == 'create':
        radio_id = create_radio(args)
        print(f"New radio added to database: {radio_id}")
        update_radio(args)
        print("Radio updated.")
    
    elif args.action == 'update':
        update_radio(args)

    elif args.action == 'write_firmware':
        if args.auto_detect_serial_port:
            port = serial_ports()[0]
        else:
            port = args.target
        print('If writing over UART, be sure to press the boot to program button', flush=True)
        esptool.main(['--chip', 'esp32s3', '--port', port, '--baud', '921600',  '--before', 
                      'default_reset', '--after', 'hard_reset', 'write_flash',  '-z', 
                      '--flash_mode', 'dio', '--flash_freq', '80m', '--flash_size', '4MB', 
                      '0x0', f'./{args.firmware_version}/firmware.ino.bootloader.bin', '0x8000', 
                      f'./{args.firmware_version}/firmware.ino.partitions.bin', '0xe000', 
                      './libs/esp32s3-2.0.14.boot_app0.bin', '0x10000', 
                      f'./{args.firmware_version}/firmware.ino.bin'])

