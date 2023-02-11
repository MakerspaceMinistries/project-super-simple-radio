from flask import Flask
from flask_restful import Api
import os
import datetime
# from endpoints.radios import RadioEndpoint, RadiosEndpoint
from endpoints.sessions import AdminSessionsEndpoint
from endpoints.admins import AdminsEndpoint
from endpoints.stations import StationEndpoint, StationsEndpoint
from endpoints.networks import NetworksEndpoint

app = Flask(__name__)

secret_key = os.environ.get("FLASK_SECRET_KEY")
app.secret_key = bytes(secret_key, "utf-8").decode("unicode_escape")
# This needs to be 50 years so that it will work with the current yard site which uses an epoch starting in 2011
app.config["PERMANENT_SESSION_LIFETIME"] = datetime.timedelta(days=365) * 50


api = Api(app)
api.init_app(app)


@app.route("/")
def hello():
    return "Hello World"


api_prefix = "/api/v1"
api.add_resource(AdminsEndpoint, api_prefix + "/admins")
api.add_resource(AdminSessionsEndpoint, api_prefix + "/admins/sessions")

api.add_resource(NetworksEndpoint, api_prefix + "/networks")
# api.add_resource(NetworksEndpoint, api_prefix+'/networks/<int:network_id>')
api.add_resource(StationsEndpoint, api_prefix + "/networks/stations")
api.add_resource(StationEndpoint, api_prefix + "/networks/<int:network_id>/stations/<int:station_id>")

# api.add_resource(RadiosEndpoint, api_prefix+'/radios')
# api.add_resource(RadioEndpoint, api_prefix+'/radios/<radio_id>')


if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)
