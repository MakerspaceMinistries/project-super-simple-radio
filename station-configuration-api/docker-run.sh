docker run -it --rm --env-file=SECRETS.sh -p 127.0.0.1:5000:5000 -v ${PWD}\app:/app projectsupersimpleradio:latest "python" "/app/main.py"


