from flask import session
from flask_restful import Resource, reqparse
from endpoints import admins_only
from database import Database

# Admin must be associated with the station's network.
get_station_query = "SELECT * FROM Stations WHERE network_id = %s AND station_id = %s AND EXISTS (SELECT * FROM AdminsNetworks WHERE network_id = %s AND user_id = %s) LIMIT 1;"


class StationsEndpoint(Resource):
    method_decorators = {"post": [admins_only]}

    def get(self):
        parser = reqparse.RequestParser()
        parser.add_argument("network_id", type=int, default=None)
        args = parser.parse_args()

        with Database() as connection:
            data = (args["network_id"], args["network_id"])
            connection.execute("SELECT * FROM Stations WHERE network_id = %s OR %s IS NULL;", data)
            stations = connection.fetch()

        return stations

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument("station_url", type=str)
        parser.add_argument("station_name", type=str)
        parser.add_argument("network_id", type=int)
        args = parser.parse_args()
        with Database() as connection:
            data = (args["station_url"], args["station_name"], args["network_id"], session["user_id"], args["network_id"])
            # Admin must be associated with the station's network.
            # This uses INSERT INTO ... SELECT instead of VALUES - https://stackoverflow.com/a/63033661
            query = "INSERT INTO Stations (station_url, station_name, network_id) SELECT %s, %s, %s WHERE EXISTS(SELECT * FROM AdminsNetworks WHERE user_id = %s AND network_id = %s);"
            connection.execute(query, data)

            station_id = connection.lastrowid()

            data = (args["network_id"], station_id, args["network_id"], session["user_id"])
            connection.execute(get_station_query, data)
            station = connection.fetch(first=True)
        return station


class StationEndpoint(Resource):
    """
    Station CRUD
    """

    method_decorators = [admins_only]

    def get(self, network_id, station_id):
        with Database() as connection:
            data = (network_id, station_id, network_id, session["user_id"])
            connection.execute(get_station_query, data)
            station = connection.fetch(first=True)
        return station

    def put(self, network_id, station_id):
        parser = reqparse.RequestParser()
        parser.add_argument("station_url", type=str)
        parser.add_argument("station_name", type=str)
        args = parser.parse_args()
        with Database() as connection:
            data = (args["station_url"], args["station_name"], station_id, session["user_id"], network_id)
            # Admin must be associated with the station's network.
            query = "UPDATE Stations SET station_url = %s, station_name = %s WHERE station_id = %s AND EXISTS (SELECT * FROM AdminsNetworks WHERE user_id = %s AND network_id = %s) LIMIT 1;"
            connection.execute(query, data)

            data = (network_id, station_id, network_id, session["user_id"])
            connection.execute(get_station_query, data)
            station = connection.fetch(first=True)
        return station

    def delete(self, network_id, station_id):
        with Database() as connection:
            data = (station_id, session["user_id"], network_id)
            # Admin must be associated with the station's network.
            query = "DELETE FROM Stations WHERE station_id = %s AND EXISTS (SELECT * FROM AdminsNetworks WHERE user_id = %s AND network_id = %s)"
            connection.execute(query, data)
        return "", 204
