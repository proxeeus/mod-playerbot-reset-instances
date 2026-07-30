SET
@Entry = 190012,
@DisplayId = 26790,
@Name = "Chronomancer Noriol";
DELETE FROM creature_template WHERE entry = @Entry;

INSERT INTO creature_template (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `rank`, `dmgschool`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `AIName`, `MovementType`, `HoverHeight`, `RacialLeader`, `movementId`, `RegenHealth`, `flags_extra`, `ScriptName`) VALUES
(@Entry, @Name, "Warden of Time", NULL, 0, 80, 80, 2, 35, 1, 0, 0, 2000, 0, 1, 0, 7, 138936390, 0, 0, 0, '', 0, 1, 0, 0, 1, 0, 'chronomancer_noriol');

DELETE FROM creature_template_model WHERE CreatureID = @Entry;
INSERT INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild) VALUES
(@Entry, 0, @DisplayId, 1, 1, 0);