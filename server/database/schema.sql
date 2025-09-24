-- backing_bot sets can_receive_updates = 0.

CREATE TABLE Telegram_Bot (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    token TEXT UNIQUE NOT NULL,
    can_receive_updates BOOLEAN NULL DEFAULT 1
);

CREATE TABLE Telegram_Message (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES Telegram_Bot(id),
    text TEXT,
    caption TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- uploaded by the client(the bot) to the chat / server, 
-- or by the user() to be handled as an update by the bot. 
CREATE TABLE Telegram_File (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL REFERENCES Telegram_Message(id) ON DELETE CASCADE,
    file_path TEXT NOT NULL,          -- e.g. storage path or virtual file_id
    mime_type TEXT,                   -- optional: "image/png", "application/pdf"
    extension TEXT,                   -- optional: "png", "pdf"
    data BLOB NOT NULL
);

CREATE TABLE Telegram_Update (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES Telegram_Bot(id),
    message_id INTEGER REFERENCES Telegram_Message(id),
    delivered BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

