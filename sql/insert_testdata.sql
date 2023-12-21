-- Insert test data into items
INSERT INTO items (name, price, description) VALUES
    ('Product A', 10000, 'Description for Product A'),
    ('Product B', 7500, 'Description for Product B'),
    ('Product C', 5000, 'Description for Product C');

-- Insert test data into orders
INSERT INTO orders (state_id) VALUES
    (1),  -- Order in 'Created' state
    (1);  -- Order in 'Created' state

-- Insert test data into order items
INSERT INTO order_items (order_id, item_id, quantity, unit_price) VALUES
    (1, 1, 2, 10000),  -- Order 1 contains 2 units of Product A
    (1, 2, 1, 7500),   -- Order 1 contains 1 unit of Product B
    (2, 2, 3, 7500),   -- Order 2 contains 3 units of Product B
    (2, 3, 1, 5000);   -- Order 2 contains 1 unit of Product C
