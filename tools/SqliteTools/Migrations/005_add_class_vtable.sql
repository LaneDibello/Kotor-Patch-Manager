-- Migration v4 → v5: Add vtable pointer address to classes
-- Optional, nullable per-class virtual table address. Populated from a Ghidra
-- export via the import-vtables command. Used at runtime when a wrapper must
-- stamp its own vtable because the in-game constructor was inlined away (e.g.
-- CSWGui3DSceneView). Not all classes have a vtable, so this stays nullable.

ALTER TABLE classes ADD COLUMN vtable INTEGER;

-- Update schema version
INSERT INTO schema_version (version) VALUES (5);
