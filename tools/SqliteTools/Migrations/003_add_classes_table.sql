-- Migration v2 → v3: Add classes table
-- Tracks per-game-version class metadata. For now only the class size is
-- stored, but the table is intended to hold additional class data later.

CREATE TABLE classes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_name TEXT NOT NULL UNIQUE,
    size INTEGER,
    notes TEXT
);

CREATE INDEX idx_classes_name ON classes(class_name);

-- Update schema version
INSERT INTO schema_version (version) VALUES (3);
