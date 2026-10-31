// Corrosion (136021): combat status created on the opponent's side by both Rifthounds
// (Thundercraven Rifthound 3409 and Rockfond Rifthound 3602).
// Each End Phase consumes 1 Usage and deals 1 Piercing DMG to every character on that side,
// except standby characters with 3 HP or less. Stacks add Usages (max 3), so the DMG lasts more rounds.
import { $, DamageType } from "@gi-tcg/core/data";

/**
 * @id 136021
 * @name Corrosion
 * @description
 * End Phase: Apart from non-active characters who have no more than 3 HP, all of characters in this play
 * area take 1 Piercing DMG.
 * Usage(s): 1 (Can stack. Max 3 stacks.)
 */
define combatStatus {
  id 136021 as Corrosion;
  on endPhase {
    usage 1 {
      append 3;
    };
    for (const character of :queryAll($.my.character)) {
      if (character.isActive() || character.health > 3) {
        :damage(DamageType.Piercing, 1, character);
      }
    }
  };
};
