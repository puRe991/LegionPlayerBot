--
-- FelForge: hotfixes database, after the LegionCore release dump
--
-- Apply to the `hotfixes` database after
-- LegionCore_hotfixes_735.26972_2020_04_03.sql. See docs/SETUP.md, section 3.
--
-- Taken from sql/updates/hotfixes/0001_fix_garrison_mission_db_structure.sql
-- of The-Legion-Preservation-Project/LegionCore-7.3.5 (GPL-2.0): the core
-- selects garr_mission.RelationshipData, the dump names it CriteriaID.
--

ALTER TABLE `garr_mission`
	CHANGE COLUMN `CriteriaID` `RelationshipData` INT(11) NOT NULL DEFAULT '0' AFTER `EnvGarrMechanicID`;
