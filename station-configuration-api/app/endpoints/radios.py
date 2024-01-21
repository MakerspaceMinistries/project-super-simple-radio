from flask import session
from flask_restful import Resource, reqparse, inputs
from mysql.connector.errors import IntegrityError
from endpoints import admins_only, is_admin
from database import Database
from flask_exception import CustomFlaskException


class RadiosEndpoint(Resource):
    method_decorators = [admins_only]

    def get(self):
        parser = reqparse.RequestParser()
        parser.add_argument("all_radios", type=bool, default=False)
        args = parser.parse_args()
        with Database() as connection:
            data = (session["user_id"], args["all_radios"])
            connection.execute(
                "SELECT * FROM Radios WHERE network_id IN (SELECT network_id FROM AdminsNetworks WHERE user_id = %s) OR %s = TRUE", data
            )
            result = connection.fetch()
        return result

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument("radio_id", type=str)
        parser.add_argument("pcb_version", type=str)
        parser.add_argument("firmware_version", type=str)
        parser.add_argument("max_station_count", type=int)
        parser.add_argument("has_channel_potentiometer", type=inputs.boolean)
        parser.add_argument("label", type=str)
        parser.add_argument("network_id", type=int)
        args = parser.parse_args()
        with Database() as connection:
            data = (
                args["radio_id"],
                args["pcb_version"],
                args["firmware_version"],
                args["max_station_count"],
                args["has_channel_potentiometer"],
                args["label"],
                args["network_id"],
                session["user_id"],
                args["network_id"],
            )
            query = "INSERT INTO Radios (radio_id, pcb_version, firmware_version, max_station_count, has_channel_potentiometer, label, network_id) SELECT %s, %s, %s, %s, %s, %s, %s WHERE EXISTS(SELECT * FROM AdminsNetworks WHERE user_id = %s AND network_id = %s);"
            try:
                connection.execute(query, data)
            except IntegrityError as e:
                if e.errno == 1062:
                    raise CustomFlaskException("This radio_id has already been used.", status_code=400)
                raise e

            lastrowid = connection.lastrowid()

            data = (args["radio_id"],)
            connection.execute("SELECT * FROM Radios WHERE radio_id = %s", data)
            radio = connection.fetch(first=True)
        return radio


class RadioEndpoint(Resource):
    # GET nad PUT are public endpoints, with the password being the radio_id
    method_decorators = {"delete": [admins_only]}

    def get(self, radio_id):
        with Database() as connection:
            # Get Radio
            data = (radio_id,)
            query = "SELECT * FROM Radios WHERE radio_id = %s LIMIT 1;"
            connection.execute(query, data)
            result = connection.fetch(first=True)

            # Get Stations
            data = (radio_id,)
            query = "SELECT Stations.station_id, network_id, station_url, station_name, position FROM RadiosStations JOIN Stations ON RadiosStations.station_id = Stations.station_id WHERE radio_id = %s ORDER BY position ASC;"
            connection.execute(query, data)
            result["stations"] = connection.fetch()

        return result

    def put(self, radio_id):
        parser = reqparse.RequestParser()
        parser.add_argument("label", type=str)
        parser.add_argument("network_id", type=int)
        parser.add_argument("show_stations_from_all_networks", type=inputs.boolean, default=False)
        parser.add_argument("station_id", action="append", type=str)
        args = parser.parse_args()

        radio = self.get(radio_id)

        radio["label"] = args["label"]
        if is_admin(session):
            radio["network_id"] = args["network_id"]
            radio["show_stations_from_all_networks"] = args["show_stations_from_all_networks"]

        with Database() as connection:
            # Update the Radios table
            data = (radio["label"], radio["network_id"], radio["show_stations_from_all_networks"], radio_id)
            query = "UPDATE Radios SET label=%s, network_id=%s, show_stations_from_all_networks=%s WHERE radio_id=%s;"
            connection.execute(query, data)

            # delete all stations associated with the radio_id
            data = (radio_id,)
            query = "DELETE FROM RadiosStations WHERE radio_id=%s;"
            connection.execute(query, data)

            # Insert stations
            data = []
            for position, station_id in enumerate(args["station_id"]):
                data.append((radio_id, station_id, position))
            query = "INSERT INTO RadiosStations (radio_id, station_id, position) VALUES (%s, %s, %s)"
            connection.executemany(query, data)

        return self.get(radio_id)

    # def delete(self, radio_id):
    #     pass
