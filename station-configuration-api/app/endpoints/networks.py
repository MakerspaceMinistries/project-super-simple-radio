from flask_restful import Resource
from database import Database
from endpoints import admins_only


class NetworksEndpoint(Resource):
    method_decorators = [admins_only]

    def get(self):
        with Database() as connection:
            connection.execute("SELECT * FROM Networks;")
            networks = connection.fetch()
        return networks
