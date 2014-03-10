--
-- This file reflects the latest changes in mod.cservice (26/12/2001)
-- so, you NEED to update your cservice database using that file if you
-- upgraded mode.cservice to a version after 26/12/2001 and you are using an older db schema.
--

ALTER TABLE channels ADD COLUMN no_take INT4 DEFAULT '0';
ALTER TABLE channels ALTER COLUMN flood_pro TYPE INT4;
UPDATE channels SET flood_pro = 0;
UPDATE channels SET no_take = 0;

