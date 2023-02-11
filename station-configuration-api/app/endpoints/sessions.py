from flask import session
from flask_restful import Resource, reqparse
from endpoints import logout, passwords_match, login
from database import Database
from flask_exception import CustomFlaskException


class AdminSessionsEndpoint(Resource):
    def get(self):
        """
        This is for testing purposes only & simply returns the current user_id or None
        """
        user_id = session["user_id"] if "user_id" in session else None
        return {"user_id": user_id}

    def post(self):
        logout()

        parser = reqparse.RequestParser()
        parser.add_argument("username", required=True, type=str)
        parser.add_argument("password", required=True, type=str)
        args = parser.parse_args()

        with Database() as connection:
            data = (args["username"],)
            connection.execute("SELECT * FROM Admins WHERE username = %s;", data)
            user = connection.fetch(first=True)

        authenticated = False
        if user:
            authenticated = passwords_match(user["password"], args["password"])

        if authenticated:
            login(user["user_id"], is_admin=True)
            return "", 204
        else:
            raise CustomFlaskException("No account was found with the provided username.", status_code=401)

    def delete(self):
        logout()
        return "", 204
