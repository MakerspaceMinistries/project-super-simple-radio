from flask import session
from flask_restful import Resource, reqparse
from database import Database
from endpoints import generate_password, get_user, admins_only


class AdminsEndpoint(Resource):
    method_decorators = [admins_only]

    def get(self):
        user_id = session["user_id"] if "user_id" in session else None
        if not user_id:
            return {"user_id": None}
        user_id = int(user_id)
        return get_user(user_id)

    def put(self):
        parser = reqparse.RequestParser()
        parser.add_argument("password", required=True, type=str)
        args = parser.parse_args()

        password = generate_password(args["password"])
        user = (password, session["user_id"])
        with Database() as connection:
            connection.execute("UPDATE Admins SET password = %s WHERE user_id = %s;", user)
        return "", 204

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument("username", required=True, type=str)
        parser.add_argument("password", required=True, type=str)
        args = parser.parse_args()

        password = generate_password(args["password"])
        data = (args["username"], password)

        with Database() as connection:
            connection.execute("INSERT INTO Admins (username, password) VALUES ( %s , %s );", data)
            user_id = connection.lastrowid()
        return user_id

    # def delete(self):
    #     return
