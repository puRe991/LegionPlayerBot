--
-- FelForge: auth database, after the LegionCore base
--
-- Apply to the `auth` database after sql/base/auth_database.sql from tag
-- LegionCore735.20041 of The-Legion-Preservation-Project/LegionCore-7.3.5,
-- and before sql/base/auth_playerbot.sql. See docs/SETUP.md, section 3.
--
-- Taken from that project's sql/updates/auth/0001_merge_account_tables.sql
-- and 0002_battlepay_cleanup.sql (GPL-2.0). The core reads balans, activate
-- and access_ip from `account` and logs donations to
-- `account_donate_token_log`; the base still has them in separate tables.
-- Their 0003_battlepay_rework.sql belongs to a later core (it renames
-- account_donate_token_log.type) and is deliberately left out.
--

-- 0001_merge_account_tables.sql
ALTER TABLE `account`
	CHANGE COLUMN `username` `username` VARCHAR(255) NOT NULL DEFAULT '' AFTER `id`,
	CHANGE COLUMN `sha_pass_hash` `sha_pass_hash` VARCHAR(512) NOT NULL DEFAULT '' AFTER `username`,
	CHANGE COLUMN `v` `v` VARCHAR(512) NOT NULL DEFAULT '' AFTER `sessionkey`,
	CHANGE COLUMN `s` `s` VARCHAR(512) NOT NULL DEFAULT '' AFTER `v`,
	ADD COLUMN `access_ip` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `last_ip`,
	ADD COLUMN `email_blocked` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `failed_logins`,
	ADD COLUMN `last_email` TIMESTAMP NULL DEFAULT NULL AFTER `last_login`,
	CHANGE COLUMN `recruiter` `recruiter` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `os`,
	ADD COLUMN `invite` VARCHAR(32) NOT NULL DEFAULT '' AFTER `recruiter`,
	ADD COLUMN `lang` ENUM('tw','cn','en','ua','ru') NOT NULL DEFAULT 'en' AFTER `invite`,
	ADD COLUMN `referer` VARCHAR(255) NOT NULL DEFAULT '' AFTER `lang`,
	ADD COLUMN `unsubscribe` VARCHAR(32) NOT NULL DEFAULT '0' AFTER `referer`,
	ADD COLUMN `dt_vote` TIMESTAMP NULL DEFAULT NULL AFTER `unsubscribe`,
	ADD COLUMN `balans` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `hwid`,
	ADD COLUMN `karma` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `balans`,
	ADD COLUMN `activate` TINYINT(1) UNSIGNED NOT NULL DEFAULT '1' AFTER `karma`,
	ADD COLUMN `verify` TINYINT(1) UNSIGNED NOT NULL DEFAULT '0' AFTER `activate`,
	ADD COLUMN `tested` TINYINT(1) UNSIGNED NOT NULL DEFAULT '0' AFTER `verify`,
	ADD COLUMN `donate` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `tested`,
	ADD COLUMN `phone` VARCHAR(255) NOT NULL DEFAULT '' AFTER `donate`,
	ADD COLUMN `phone_hash` VARCHAR(32) NOT NULL DEFAULT '' AFTER `phone`,
	ADD COLUMN `telegram_lock` TINYINT(1) UNSIGNED NOT NULL DEFAULT '0' AFTER `phone_hash`,
	ADD COLUMN `telegram_id` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `telegram_lock`,
	DROP COLUMN `battlenet_account`,
	DROP COLUMN `battlenet_index`,
	DROP INDEX `bnet_acc`,
	DROP INDEX `battlenet_account`,
	DROP INDEX `battlenet_index`;

-- Changed for MariaDB, which refuses to drop a column that is part of a
-- multi-column key: rebuild the key without it.
ALTER TABLE `account_rates`
	DROP INDEX `unique`,
	DROP COLUMN `bnet_account`,
	ADD UNIQUE KEY `unique` (`account`, `realm`);

UPDATE account INNER JOIN battlenet_accounts ON account.username = battlenet_accounts.email SET account.sha_pass_hash = battlenet_accounts.sha_pass_hash;

DROP TABLE `battlenet_accounts`;
DROP TABLE `battlenet_account_bans`;

-- 0002_battlepay_cleanup.sql
DROP TABLE `store_statistics`;
DROP TABLE `store_history`;

CREATE TABLE `account_donate_token_log` (
	`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
	`time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(),
	`accountId` INT(10) UNSIGNED NOT NULL,
	`realmId` INT(10) UNSIGNED NOT NULL,
	`characterId` BIGINT(20) UNSIGNED NOT NULL,
	`change` INT(11) NOT NULL,
	`type` TINYINT(3) UNSIGNED NOT NULL,
	`productId` INT(10) UNSIGNED NOT NULL,
	PRIMARY KEY (`id`)
)
COLLATE='latin1_swedish_ci';

ALTER TABLE `account`
	CHANGE COLUMN `balans` `balans` INT(11) NOT NULL DEFAULT '0' AFTER `hwid`;
