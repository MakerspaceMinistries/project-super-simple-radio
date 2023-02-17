FROM python:3.11-buster

RUN apt-get update && apt-get upgrade -y

RUN python -m pip install --upgrade pip

COPY ./app/requirements.txt /app/requirements.txt

RUN pip install -r /app/requirements.txt

COPY ./app /app

WORKDIR /app

RUN chmod +x gunicorn.sh

EXPOSE 80

CMD ["/app/gunicorn.sh"]
