from flask_restful import Resource, reqparse, inputs
from database import Database


class RadioDeviceInterface_v1_0_Endpoint(Resource):
    def get(self, radio_id):
        parser = reqparse.RequestParser()
        parser.add_argument("pcb_version", type=str)
        parser.add_argument("firmware_version", type=str)
        parser.add_argument("max_station_count", type=int)
        parser.add_argument("has_channel_potentiometer", type=inputs.boolean)
        parser.add_argument("update_last_seen", type=inputs.boolean, default=True)
        args = parser.parse_args()

        with Database() as connection:
            last_seen = ", last_seen=NOW() " if args["update_last_seen"] else " "
            data = (
                args["pcb_version"],
                args["firmware_version"],
                args["max_station_count"],
                args["has_channel_potentiometer"],
                radio_id,
            )
            query = (
                "UPDATE Radios SET pcb_version=%s, firmware_version=%s, max_station_count=%s, has_channel_potentiometer=%s"
                + last_seen
                + "WHERE radio_id=%s;"
            )
            connection.execute(query, data)

            data = (radio_id,)
            query = "SELECT station_url FROM RadiosStations JOIN Stations ON RadiosStations.station_id = Stations.station_id WHERE radio_id = %s ORDER BY position ASC;"
            connection.execute(query, data)
            stations = connection.fetch()

            data = (radio_id,)
            query = "SELECT * FROM Radios WHERE radio_id = %s;"
            connection.execute(query, data)
            radio = connection.fetch(first=True)

        station_urls = [s["station_url"] for s in stations]

        response = {"stationCount": len(station_urls)}

        stationsKeys = [ "stn" + str(i) + "URL" for i in range(1,10) ]
        for i, key in enumerate(stationsKeys[0:radio["max_station_count"]]):
            response[key] = station_urls[i] if i < len(station_urls) else ""

        return response
