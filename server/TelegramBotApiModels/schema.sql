-- backing_bot sets can_receive_updates = 0.

CREATE TABLE telegram_bot (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    token TEXT UNIQUE NOT NULL,
    can_receive_updates BOOLEAN
);

CREATE TABLE telegram_message (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
    text TEXT,
    caption TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE telegram_file (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL REFERENCES telegram_message(id) ON DELETE CASCADE,
    file_path TEXT NOT NULL,          -- e.g. storage path or virtual file_id
    mime_type TEXT,                   -- optional: "image/png", "application/pdf"
    extension TEXT,                   -- optional: "png", "pdf"
    data BLOB NOT NULL
);

CREATE TABLE telegram_update (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
    message_id INTEGER REFERENCES telegram_message(id),
    delivered BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

