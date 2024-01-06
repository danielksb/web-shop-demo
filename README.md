# Development

Start Postgres
~~~
docker compose up -d
~~~

# Notes


Results can also be return as binary by PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, 1);
The return value of PQgetvalue() needs to be casted then.

Sending random strings to echo server
~~~sh
for i in {1..80}; do echo -n "$(head /dev/urandom | tr -dc 'a-zA-Z0-9' | head -c 32)\n" | nc localhost 8080 & done
~~~