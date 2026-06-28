-- Migration v3 → v4: Add platform to game_version
-- Records the OS/platform the game build targets (e.g. Windows, MacOS). Used at
-- runtime to select version-specific layout data such as vtable sizes. All
-- currently-known builds are Windows.

ALTER TABLE game_version ADD COLUMN platform TEXT;

UPDATE game_version SET platform = 'Windows';

-- Update schema version
INSERT INTO schema_version (version) VALUES (4);
