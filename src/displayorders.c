#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

void printResults(PGresult *result) {
    int rows, cols, i, j;

    rows = PQntuples(result);
    cols = PQnfields(result);

    // Print column headers
    for (i = 0; i < cols; i++) {
        printf("%-20s", PQfname(result, i));
    }
    printf("\n");

    // Print rows
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            printf("%-20s", PQgetvalue(result, i, j));
        }
        printf("\n");
    }
}

int main() {
    PGconn *conn;
    PGresult *result;

    // Connect to the PostgreSQL database
    conn = PQconnectdb("dbname=shopdb user=shopuser password=shopuser host=localhost port=5432");

    // Check if the connection was successful
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    // Execute the SQL query
    result = PQexec(conn,
        "SELECT"
        "  o.order_id,"
        "  o.order_date,"
        "  os.state_name AS order_status,"
        "  oi.order_item_id,"
        "  i.name AS item_name,"
        "  oi.quantity,"
        "  oi.unit_price"
        " FROM orders o"
        " JOIN order_items oi ON oi.order_id = o.order_id"
        " JOIN order_states os ON os.state_id = o.state_id"
        " JOIN items i ON i.item_id = oi.item_id"
        " ORDER BY o.order_date DESC;");

    // Check if the query was successful
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQresultErrorMessage(result));
        PQclear(result);
        PQfinish(conn);
        return 1;
    }

    // Print the query results
    printResults(result);

    // Free the result and close the connection
    PQclear(result);
    PQfinish(conn);

    return 0;
}
