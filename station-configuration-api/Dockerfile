FROM python:3.11-buster

RUN apt-get update && apt-get upgrade -y

RUN python -m pip install --upgrade pip

COPY ./app /app

WORKDIR /app

RUN chmod +x gunicorn.sh

RUN pip install -r requirements.txt

EXPOSE 80

CMD ["/app/gunicorn.sh"]
