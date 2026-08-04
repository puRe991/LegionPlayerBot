--
-- LegionPlayerBot: custom tables for the auth database
--
-- The core queries these in src/common/Database/Implementation/LoginDatabase.cpp
-- and src/server/game/Server/ToolSocket.cpp. Without them the playerbot
-- subsystem starts with no name pool and the tool socket rejects every peer.
--
-- Apply to the `auth` database.
--

-- ---------------------------------------------------------------------------
-- toolip
--
-- Allow list for the remote tool socket. ToolSocket::Start looks the peer's
-- address up here and only then marks the connection authorised, so an empty
-- table means no host may connect.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `toolip`;
CREATE TABLE `toolip` (
  `ip` VARCHAR(45) NOT NULL COMMENT 'Peer address allowed to open a tool connection (IPv4 or IPv6)',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'Free text, for the administrator',
  PRIMARY KEY (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Hosts allowed to use the remote tool socket';

-- Loopback only by default. Add further hosts deliberately: the socket
-- performs no credential check beyond this list.
INSERT INTO `toolip` (`ip`, `comment`) VALUES
  ('127.0.0.1', 'localhost');

-- ---------------------------------------------------------------------------
-- playerbot_names
--
-- Name pool for generated bot characters. PlayerBotMgr::RandomName picks an
-- entry at random; PlayerBotMgr::InitializeCreatePlayerBotName reads the first
-- column only.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `playerbot_names`;
CREATE TABLE `playerbot_names` (
  `name` VARCHAR(12) NOT NULL COMMENT 'Character name, must satisfy the client name rules',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Name pool for generated playerbot characters';

INSERT INTO `playerbot_names` (`name`) VALUES
  ('Aedan'),('Aerin'),('Alaric'),('Alwin'),('Amara'),('Anwen'),('Arden'),('Arwyn'),
  ('Ashlin'),('Astrid'),('Baelor'),('Bardan'),('Belara'),('Brann'),('Brynn'),('Cadoc'),
  ('Caelis'),('Calder'),('Carwyn'),('Cedrik'),('Ceridan'),('Corvin'),('Cyrion'),('Dagen'),
  ('Darion'),('Delwyn'),('Doran'),('Draven'),('Edran'),('Eiluned'),('Elandra'),('Elric'),
  ('Emeric'),('Eowyn'),('Erevan'),('Fenwick'),('Fioran'),('Galen'),('Gareth'),('Gwenna'),
  ('Hadrian'),('Halric'),('Ilyana'),('Isolde'),('Jareth'),('Kaelen'),('Kerrin'),('Korvan'),
  ('Lirien'),('Lorcan'),('Lyanna'),('Maevis'),('Marden'),('Merrin'),('Mordain'),('Nerys'),
  ('Nolwen'),('Oriane'),('Perrin'),('Quorin'),('Rhydian'),('Rowena'),('Sarien'),('Selwyn'),
  ('Sorcha'),('Tamsin'),('Theron'),('Torvald'),('Ulric'),('Vaelin'),('Verena'),('Wyndham'),
  ('Yorick'),('Zephyr');

-- ---------------------------------------------------------------------------
-- playerbot_arena
--
-- Name pool for generated arena teams. Read by
-- PlayerBotMgr::InitializeCreatePlayerBotName alongside the character names.
-- ---------------------------------------------------------------------------
DROP TABLE IF EXISTS `playerbot_arena`;
CREATE TABLE `playerbot_arena` (
  `name` VARCHAR(24) NOT NULL COMMENT 'Arena team name',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Name pool for generated playerbot arena teams';

INSERT INTO `playerbot_arena` (`name`) VALUES
  ('Ashen Vanguard'),('Bloodhawk Company'),('Crimson Verdict'),('Dawnbreakers'),
  ('Emberfall'),('Frostwake'),('Gilded Fangs'),('Hollow Crown'),('Ironwake'),
  ('Kingsmourne'),('Lanternguard'),('Mistvale Sentinels'),('Nightreach'),
  ('Oathkeepers'),('Pale Ascent'),('Quietstorm'),('Ravenhold'),('Stormcallers'),
  ('Thornwatch'),('Umbral Pact'),('Valorbound'),('Wintermourne'),('Yielding Tide'),
  ('Zealots of Dawn');
