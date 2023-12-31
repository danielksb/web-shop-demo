CREATE TABLE items (
    item_id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    price INTEGER NOT NULL,
    description TEXT
);

CREATE TABLE order_states (
    state_id SERIAL PRIMARY KEY,
    state_name VARCHAR(50) NOT NULL
);

INSERT INTO order_states (state_name) VALUES
    ('created'),
    ('ordered'),
    ('shipped');

CREATE TABLE orders (
    order_id SERIAL PRIMARY KEY,
    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    state_id INTEGER REFERENCES order_states(state_id)
);

CREATE TABLE order_items (
    order_item_id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES orders(order_id),
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER NOT NULL,
    unit_price INTEGER NOT NULL,
    CONSTRAINT unique_order_item_order_id UNIQUE (order_id, order_item_id)
);
