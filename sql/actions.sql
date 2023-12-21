-- Insert a new order
INSERT INTO orders (state_id) VALUES
    (1);  -- New order in 'Created' state

-- Insert items for the new order
-- Assuming you have products with item_id 1 (Product A) and 3 (Product C)
INSERT INTO order_items (order_id, item_id, quantity, unit_price) VALUES
    (3, 1, 3, 100),  -- New order contains 3 units of Product A
    (3, 3, 1, 50);   -- New order contains 1 unit of Product C

-- Retrieve all orders sorted by date with the latest orders at the top
SELECT
    o.order_id,
    o.order_date,
    os.state_name AS order_status,
    oi.order_item_id,
    i.name AS item_name,
    oi.quantity,
    oi.unit_price
FROM orders o
JOIN order_items oi ON oi.order_id = o.order_id
JOIN order_states os ON os.state_id = o.state_id
JOIN items i ON i.item_id = oi.item_id
ORDER BY o.order_date DESC;

select * from orders;