--
-- LegionPlayerBot: custom tables for the world database
--
-- Queried from src/common/Database/Implementation/WorldDatabase.cpp and
-- src/server/game/PlayerBot/. Without `aiwaypoints` the bots have no
-- navigation graph, which is what makes them behave passively in
-- battlegrounds.
--
-- Apply to the `world` database.
--

-- ---------------------------------------------------------------------------
-- aiwaypoints
--
-- Navigation graph for the bot movement code. AIWaypointsMgr reads the whole
-- table on startup and writes back through the WORLD_INS/UPD/DEL_AIWAYPOINTS
-- statements.
--
-- `link` holds a comma separated list of neighbouring waypoint entries, built
-- by AIWaypoint::NewToDatabase. AIWaypointsMgr also reads AUTO_INCREMENT for
-- this table out of information_schema to allocate the next entry id, so the
-- column has to stay AUTO_INCREMENT.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `aiwaypoints`;
CREATE TABLE `aiwaypoints` (
  `entry`    INT UNSIGNED    NOT NULL AUTO_INCREMENT COMMENT 'Waypoint id',
  `map`      INT UNSIGNED    NOT NULL DEFAULT 0      COMMENT 'Map id the waypoint belongs to',
  `x`        FLOAT           NOT NULL DEFAULT 0      COMMENT 'World position X',
  `y`        FLOAT           NOT NULL DEFAULT 0      COMMENT 'World position Y',
  `z`        FLOAT           NOT NULL DEFAULT 0      COMMENT 'World position Z',
  `link`     VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT 'Comma separated entries of the linked waypoints',
  `helpText` VARCHAR(255)    NOT NULL DEFAULT ''     COMMENT 'Free text describing the point, for editing',
  PRIMARY KEY (`entry`),
  KEY `idx_map` (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Navigation graph used by the playerbot movement code';

-- Battleground objective points. The objective code addresses these by entry
-- id, hard coded in src/server/game/Battlegrounds/CommandBG/Command*.cpp and
-- ArenaDalaranSewers.cpp, so the ids must stay exactly as they are. Taken from
-- bot_world.sql in sql/justbot.rar; descriptions translated from Chinese.
-- There is no in-game command to create waypoints, so without these rows the
-- objective code has nothing to walk to.
INSERT INTO `aiwaypoints` (`entry`, `map`, `x`, `y`, `z`, `link`, `helpText`) VALUES
  -- Warsong Gulch (489)
  (7,  489, 1539.39, 1481.34, 352.487, '', 'Warsong Gulch: alliance flag'),
  (8,  489, 1449.07, 1491, 350.279, '', 'Warsong Gulch: alliance exit 1'),
  (9,  489, 1355.67, 1461.9, 324.513, '', 'Warsong Gulch: alliance exit 2'),
  (10, 489, 1356.13, 1396.79, 325.811, '', 'Warsong Gulch: alliance exit 3'),
  (11, 489, 916.729, 1434.61, 346.283, '', 'Warsong Gulch: horde flag'),
  (12, 489, 1003.91, 1425.61, 344.184, '', 'Warsong Gulch: horde exit 1'),
  (13, 489, 1120.45, 1462.24, 316.268, '', 'Warsong Gulch: horde exit 2'),
  (14, 489, 1094.43, 1532.12, 315.771, '', 'Warsong Gulch: horde exit 3'),
  -- Arathi Basin (529)
  (17, 529, 1166.28, 1202.05, -56.4207, '', 'Arathi Basin: stables'),
  (18, 529, 855.425, 1150.35, 11.539, '', 'Arathi Basin: lumber mill'),
  (19, 529, 978.243, 1045.1, -44.5344, '', 'Arathi Basin: blacksmith'),
  (20, 529, 1145.71, 849.484, -110.513, '', 'Arathi Basin: gold mine'),
  (21, 529, 804.227, 874.871, -55.2496, '', 'Arathi Basin: farm'),
  (22, 529, 1154.43, 999.954, -63.9715, '', 'Arathi Basin: alliance bridge'),
  (23, 529, 850.353, 986.463, -60.0914, '', 'Arathi Basin: horde bridge'),
  (24, 529, 1291.84, 1289.03, -13.6839, '', 'Arathi Basin: alliance start'),
  (25, 529, 701.937, 703.624, -15.9786, '', 'Arathi Basin: horde start'),
  -- Eye of the Storm (566)
  (26, 566, 2456.89, 1602.02, 1206.45, '', 'Eye of the Storm: alliance start'),
  (27, 566, 1875.77, 1530.65, 1206.87, '', 'Eye of the Storm: horde start'),
  (28, 566, 2284.83, 1730.77, 1189.91, '', 'Eye of the Storm: alliance side tower 1 (Mage Tower)'),
  (29, 566, 2286.77, 1402.8, 1197.18, '', 'Eye of the Storm: alliance side tower 2 (Draenei Ruins)'),
  (30, 566, 2048.39, 1393.9, 1194.39, '', 'Eye of the Storm: horde side tower 1 (Blood Elf Tower)'),
  (31, 566, 2044, 1729.95, 1189.86, '', 'Eye of the Storm: horde side tower 2 (Fel Reaver Ruins)'),
  (32, 566, 2174.32, 1569.64, 1159.96, '', 'Eye of the Storm: flag'),
  -- Alterac Valley (30)
  (37, 30, 705.506, -15.6266, 50.1454, '', 'Alterac Valley: alliance general'),
  (38, 30, 638.783, -33.4612, 45.9414, '', 'Alterac Valley: alliance general graveyard'),
  (39, 30, 673.537, -143.997, 63.6631, '', 'Alterac Valley: alliance north bunker'),
  (40, 30, 553.358, -77.6423, 51.9391, '', 'Alterac Valley: alliance south bunker'),
  (41, 30, 667.742, -293.749, 30.301, '', 'Alterac Valley: Stormpike graveyard'),
  (42, 30, 202.546, -359.454, 56.387, '', 'Alterac Valley: Icewing bunker'),
  (43, 30, -152.278, -440.811, 40.3996, '', 'Alterac Valley: Stonehearth bunker'),
  (44, 30, 77.2176, -402.11, 46.407, '', 'Alterac Valley: Stonehearth graveyard'),
  (45, 30, -41.3935, -289.563, 15.0886, '', 'Alterac Valley: alliance captain'),
  (46, 30, -203.972, -114.35, 79.7015, '', 'Alterac Valley: Snowfall graveyard'),
  (47, 30, -534.268, -169.448, 57.011, '', 'Alterac Valley: horde captain'),
  (48, 30, -613.005, -397.793, 60.8684, '', 'Alterac Valley: Iceblood graveyard'),
  (49, 30, -571.105, -263.464, 75.0179, '', 'Alterac Valley: Iceblood tower'),
  (50, 30, -768.377, -362.685, 90.906, '', 'Alterac Valley: Tower Point'),
  (51, 30, -1080.68, -345.853, 55.1093, '', 'Alterac Valley: Frostwolf graveyard'),
  (52, 30, -1303.43, -316.12, 113.877, '', 'Alterac Valley: horde east tower'),
  (53, 30, -1298.56, -266.924, 114.16, '', 'Alterac Valley: horde west tower'),
  (54, 30, -1402.5, -309.193, 89.3977, '', 'Alterac Valley: horde general graveyard'),
  (55, 30, -1366.28, -231.521, 98.4241, '', 'Alterac Valley: horde general'),
  (56, 30, -252.194, -291.924, 6.67758, '', 'Alterac Valley: centre'),
  (57, 30, 795.509, -493.479, 99.7655, '', 'Alterac Valley: alliance start'),
  (58, 30, -1387.45, -550.928, 54.9859, '', 'Alterac Valley: horde start'),
  -- Isle of Conquest (628)
  (59, 628, 299.652, -785.527, 49.5924, '', 'Isle of Conquest: alliance keep graveyard'),
  (60, 628, 397.03, -859.237, 49.3704, '', 'Isle of Conquest: alliance keep teleporter, inside'),
  (61, 628, 425.63, -856.905, 48.9807, '', 'Isle of Conquest: alliance keep teleporter, outside'),
  (62, 628, 776.382, -803.496, 6.4213, '', 'Isle of Conquest: workshop graveyard'),
  (63, 628, 726.47, -360.966, 17.8253, '', 'Isle of Conquest: docks graveyard'),
  (64, 628, 807.491, -1000.85, 132.391, '', 'Isle of Conquest: hangar graveyard'),
  (65, 628, 1142.98, -779.294, 49.1, '', 'Isle of Conquest: horde keep teleporter, outside'),
  (66, 628, 1159.08, -746.338, 49.0988, '', 'Isle of Conquest: horde keep teleporter, inside'),
  (67, 628, 1283.86, -705.834, 48.9248, '', 'Isle of Conquest: horde keep graveyard'),
  (68, 628, 393.072, -833.883, 48.6395, '', 'Isle of Conquest: alliance start'),
  (69, 628, 1166.72, -763.092, 48.6384, '', 'Isle of Conquest: horde start'),
  (70, 628, 443.388, -832.336, 44.0983, '', 'Isle of Conquest: alliance siege start'),
  (71, 628, 1119.95, -761.947, 47.8879, '', 'Isle of Conquest: horde siege start'),
  -- Dalaran Sewers arena (617)
  (72, 617, 1255.58, 766.713, 3.15722, '', 'Dalaran Sewers: alliance point'),
  (73, 617, 1328.29, 814.895, 3.16107, '', 'Dalaran Sewers: horde point');

-- ---------------------------------------------------------------------------
-- bottalktext
--
-- Ambient chat lines. Grouped by talktype, with subtype selecting a variant
-- inside the group (PlayerBotTalkMgr::InitializeTalkText).
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `bottalktext`;
CREATE TABLE `bottalktext` (
  `entry`    INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Row id',
  `talktype` INT UNSIGNED NOT NULL DEFAULT 0      COMMENT 'Talk group',
  `subtype`  INT UNSIGNED NOT NULL DEFAULT 0      COMMENT 'Variant inside the group',
  `text`     TEXT         NOT NULL                COMMENT 'Line the bot says',
  PRIMARY KEY (`entry`),
  KEY `idx_talktype` (`talktype`, `subtype`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Ambient chat lines for playerbots';

-- ---------------------------------------------------------------------------
-- talkstory
--
-- Longer scripted lines used by FieldBotMgr::StartStoryTalk.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `talkstory`;
CREATE TABLE `talkstory` (
  `entry` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Row id',
  `story` TEXT         NOT NULL                COMMENT 'Story line',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Scripted story lines for field bots';

-- ---------------------------------------------------------------------------
-- playerbot_online
--
-- Bookkeeping of which bot accounts are considered online. The shipped code
-- addressed this table and its account column by their Chinese names
-- (`_假人_在线假人`.`账号ID`) in src/server/scripts/Custom/_Bot.cpp. The table is
-- created under an ASCII name here and the query was changed to match; the
-- view below keeps an existing database with the original name working.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `playerbot_online`;
CREATE TABLE `playerbot_online` (
  `accountId` INT UNSIGNED NOT NULL COMMENT 'Account id of the bot that should come online',
  PRIMARY KEY (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Bot accounts to bring online on startup';
