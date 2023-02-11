import mysql.connector
from mysql.connector import FieldType
import os

WEBSITE_DB_HOST = os.environ.get("WEBSITE_DB_HOST")
WEBSITE_DB_USER = os.environ.get("WEBSITE_DB_USER")
WEBSITE_DB_PASS = os.environ.get("WEBSITE_DB_PASS")
WEBSITE_DB_DATABASE = os.environ.get("WEBSITE_DB_DATABASE")


class Database(object):
    def __init__(self, autocommit=True):
        self.autocommit = autocommit

    def __enter__(self):
        self.connection = mysql.connector.connect(
            host=WEBSITE_DB_HOST,
            user=WEBSITE_DB_USER,
            password=WEBSITE_DB_PASS,
            database=WEBSITE_DB_DATABASE,
            autocommit=self.autocommit,
        )
        self.cursor = self.connection.cursor()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.connection:
            self.cursor.close()
            self.connection.close()

    def _parse_row(self, row):
        row = list(row)
        column_names = self.cursor.column_names
        description = self.cursor.description
        for i, item in enumerate(row):
            # Full List of field types here: https://github.com/mysql/mysql-connector-python/blob/d1e89fd3d7391084cdf35b0806cb5d2a4b413654/lib/mysql/connector/constants.py#L162
            if description[i][1] in [FieldType.DATETIME]:
                row[i] = row[i].isoformat() if row[i] is not None else None
            if description[i][1] in [FieldType.NEWDECIMAL, FieldType.DECIMAL]:
                row[i] = float(row[i]) if row[i] is not None else None
            if description[i][1] in [FieldType.TINY]:
                row[i] = True if row[i] == 1 else False
        return dict(zip(column_names, row))

    def execute(self, query, data=[]):
        self.cursor.execute(query, data)

    def fetch(self, first=False):
        ret_val = []
        for r in self.cursor:
            ret_val.append(self._parse_row(r))
        if first:
            return ret_val[0] if len(ret_val) > 0 else None
        return ret_val

    def lastrowid(self):
        return self.cursor.lastrowid
