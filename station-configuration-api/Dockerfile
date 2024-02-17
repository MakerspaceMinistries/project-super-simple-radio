FROM python:3.11-buster

RUN apt-get update && apt-get upgrade -y

RUN python -m pip install --upgrade pip

COPY ./app/requirements.txt /app/requirements.txt

RUN pip install -r /app/requirements.txt

COPY ./app /app

WORKDIR /app

EXPOSE 80

CMD ["gunicorn", "--chdir", "/app", "main:app", "-w", "5", "-b", "0.0.0.0:80", "--timeout", "3600"]
