from flask import session, abort
from werkzeug.security import generate_password_hash, check_password_hash
import os
from functools import wraps


def admins_only(f):
    """

    Endpoint decorator that requires a user to be and admin

    """

    @wraps(f)
    def decorated_function(*args, **kwargs):
        if "is_admin" in session and session["is_admin"]:
            return f(*args, **kwargs)
        else:
            return abort(401)

    return decorated_function


def logout():
    if "user_id" in session.keys():
        session.pop("user_id", None)


def salt_password(password):
    return password + os.environ.get("PASSWORD_SALT")


def login(user_id, is_admin=False):
    session["user_id"] = user_id
    if is_admin:
        session["is_admin"] = True


def passwords_match(hashed, plain):
    return check_password_hash(hashed, salt_password(plain))


def authenticate(password, username):
    with Database() as connection:
        connection.execute("SELECT * FROM Admins WHERE username = %s LIMIT 1;", (username,))
        user = connection.fetch(first=True)
    if user:
        if passwords_match(user["password"], password):
            return user["user_id"]
    return None


def generate_password(password):
    return generate_password_hash(salt_password(password), method="sha256")


def get_user(user_id):
    with Database() as connection:
        connection.execute("SELECT * FROM Admins WHERE user_id = %s LIMIT 1;", (user_id,))
        user = connection.fetch(first=True)
    return user
