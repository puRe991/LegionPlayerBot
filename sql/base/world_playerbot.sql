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
