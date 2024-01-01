import serial
import json
import uuid
import argparse
import requests
from time import sleep

parser = argparse.ArgumentParser(description='Command line radio configuration.')

parser.add_argument('-d', '--device', default='/dev/ttyACM0', type=str, help='Device to write to. Default: /dev/ACM0')
parser.add_argument('-H', '--host', type=str, help='Host of the radio configuration server.') 
parser.add_argument('-a', '--api-version',  default='v1', type=str, help='Version of the api to use. Default: v1') 
parser.add_argument('-i', '--radio-id', type=str, help='ID of the radio. If blank, a radio ID will be created.')
parser.add_argument('-c', '--config-endpoint-version', type=str, default='v1.0', help='v1.0')
parser.add_argument('-u', '--user', type=str, help='User name for the configuration server.')
parser.add_argument('-p', '--password', type=str, help='Password for the configuration server.')
parser.add_argument('-w', '--wifi', action='store_true', help='Configure the radio to connect to the default WiFi Network.')
parser.add_argument('-z', '--zebra', action='store_true', help='Generate zebra code for a QR code.')
parser.add_argument('-n', '--network', type=int, help='network_id of the network to which the radio belongs.')
parser.add_argument('-l', '--label', type=str, help='Label for the radio.')
parser.add_argument('-b', '--pcb-version', type=str, help='PCB version.')
parser.add_argument('-r', '--remote-config-background-retrieval-interval', default=43200000, type=int, help='remote_config_background_retrieval_interval')
parser.add_argument('-e', '--has-channel-pot', default=True, type=bool, help='has_channel_pot')

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
    with requests.Session() as session:

        radio_id = str(uuid.uuid1())

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
                raise Exception("Failed to create radio")
            
            return radio_id
        
        else:
            raise Exception("Failed to log in")

def write_to_serial(port, payload):
    # Open the serial port
    with serial.Serial(port, 115200, timeout=1) as ser:
        # Check if the payload is a dictionary
        if isinstance(payload, dict):
            # Convert the dictionary to a JSON string
            payload = json.dumps(payload)
        
        # Write the payload to the serial port
        # The encode() method converts the string to bytes, which is required by the write() method
        print(f"Writing: {payload}")
        ser.write(payload.encode())
        ser.flush()



def write_key_value_to_serial(port, key, value):
    kv = {key:value}
    write_to_serial(port, kv)

if __name__ == '__main__':

    if not args.radio_id:
        args.radio_id = create_radio(args)
        print(f"New radio id created: {args.radio_id}")

    write_key_value_to_serial(args.device, "remote_config",  True)
    write_key_value_to_serial(args.device, "radio_id", args.radio_id)
    write_key_value_to_serial(args.device, "remote_cfg_url",  f"{args.host}/api/{args.api_version}/radios/device_interface/{args.config_endpoint_version}/")
    write_key_value_to_serial(args.device, "has_channel_pot",  args.has_channel_pot)
    write_key_value_to_serial(args.device, "pcb_version", args.pcb_version )
    write_key_value_to_serial(args.device, "remote_config_background_retrieval_interval", args.remote_config_background_retrieval_interval )

    read_config(args.device)

    if args.wifi:
        wifi_creds = {"ssid": "radio-setup", "pass": "supersimpleradio"}
        write_to_serial(args.device, wifi_creds)

    if args.zebra:
        pass



