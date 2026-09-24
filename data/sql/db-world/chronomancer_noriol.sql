SET
@Entry = 190012,
@DisplayId = 26790,
@Name = 'Chronomancer Noriol',
@TextIntro = 190012,
@TextDescription = 190013;

DELETE FROM `creature_template` WHERE `entry` = @Entry;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `rank`, `dmgschool`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `AIName`, `MovementType`, `HoverHeight`, `RacialLeader`, `movementId`, `RegenHealth`, `flags_extra`, `ScriptName`) VALUES
(@Entry, @Name, 'Warden of Time', NULL, 0, 80, 80, 2, 35, 1, 0, 0, 2000, 0, 1, 0, 7, 138936390, 0, 0, 0, '', 0, 1, 0, 0, 1, 0, 'chronomancer_noriol');

DELETE FROM `creature_template_model` WHERE `CreatureID` = @Entry;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(@Entry, 0, @DisplayId, 1, 1, 0);

DELETE FROM `npc_text` WHERE `ID` IN (@TextIntro, @TextDescription);
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`) VALUES
(@TextIntro, 'Time is a spiral. Care to rewind your fate?', 'Time is a spiral. Care to rewind your fate?', 1),
(@TextDescription, 'Chronomancer Noriol gazes at you, eyes reflecting infinite timelines.$B$B“I offer more than magic. I offer freedom from repetition, shortcuts through weariness, and a chance to rewrite what once was. Whether to reset what you’ve done, lift your allies, or leap past the desolate lands of Outland... All I ask is a modest fee — and the will to defy time.”', 'Chronomancer Noriol gazes at you, eyes reflecting infinite timelines.$B$B“I offer more than magic. I offer freedom from repetition, shortcuts through weariness, and a chance to rewrite what once was. Whether to reset what you’ve done, lift your allies, or leap past the desolate lands of Outland... All I ask is a modest fee — and the will to defy time.”', 1);
