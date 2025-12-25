-- Migration v1 → v2: Add calling convention and parameter size to functions table

-- Add new columns (NULL allowed for backward compatibility)
ALTER TABLE functions ADD COLUMN calling_convention TEXT;
ALTER TABLE functions ADD COLUMN param_size_bytes INTEGER;

-- Update schema version
INSERT INTO schema_version (version) VALUES (2);
