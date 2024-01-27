docker run -it --rm --env-file=SECRETS.sh -p 127.0.0.1:5000:5000 -v ${PWD}/app:/app jaketeater/super-simple-radio-station-config-api:latest "python" "/app/main.py"


