# Development

Start Postgres
~~~
docker compose up -d
~~~

# Notes


Results can also be return as binary by PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, 1);
The return value of PQgetvalue() needs to be casted then.