from flask_restful import Resource
from database import Database


class RadiosEndpoint(Resource):
    def get(self):
        with Database() as connection:
            connection.execute("SELECT * FROM Radios")
            result = connection.fetch(one=True)
        return result


class RadioEndpoint(Resource):
    def get(self, radio_id):
        with Database() as connection:
            connection.execute("SELECT * FROM Radios")
            result = connection.fetch(one=True)
        return result
