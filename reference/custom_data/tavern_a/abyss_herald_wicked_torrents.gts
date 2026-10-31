// Abyss Herald: Wicked Torrents (4206): Tavern Challenge version of Piovium's 2203 with an extra passive,
// Torrential Ward: when Watery Rebirth triggers, the Herald gains Surging Ward (immune to DMG while its
// 2 Hydro applications last). Breaking the ward leaves Abyssal Tides (no Skills, 6 Piercing DMG at End Phase).
// Piovium only reserves 122035 and 122036, so they are defined here. The hidden passive Broken Ward (22037,
// also only reserved in Piovium) is not needed: Surging Ward itself attaches Abyssal Tides when it breaks.
import { Aura, DamageType } from "@gi-tcg/core/data";
import {
  RipplingSlash,
  VortexEdge,
  TorrentialShock,
  WateryRebirth,
  RipplingBlades,
  WateryRebirth01,
  WateryRebirthStatus,
} from "../../characters/hydro/abyss_herald_wicked_torrents.gts";

/**
 * @id 122036
 * @name Abyssal Tides
 * The character cannot use Skills. End Phase: deal 6 Piercing DMG to the character and remove this effect.
 */
define status {
  id 122036 as AbyssalTides;
  tags disableSkill;
  on endPhase {
    :damage(DamageType.Piercing, 6, :self.master);
    :dispose();
  };
};

/**
 * @id 122035
 * @name Surging Ward
 * The character is immune to all DMG. Provides 2 instances of Hydro Application, depleted by Elemental
 * Reactions: once depleted, this effect is removed and the character gains Abyssal Tides.
 */
define status {
  id 122035 as SurgingWard;
  variable hydroApplications, 2;
  on decreaseDamaged {
    :e.decreaseDamage(:e.value);
  };
  on reaction {
    :addVariable("hydroApplications", -1);
    if (:getVariable("hydroApplications") <= 0) {
      :characterStatus(AbyssalTides, :self.master);
      :dispose();
    } else {
      :self.master.setVariable("aura", Aura.Hydro);
    }
  };
};

/**
 * @id 22036
 * @name Torrential Ward
 * (Passive) When this character triggers Watery Rebirth: attach Surging Ward to them.
 */
define skill {
  id 22036 as TorrentialWard;
  skillType passive {
    on entityDispose {
      when :( :e.entity.definition.id === WateryRebirthStatus );
      :characterStatus(SurgingWard, :self);
      :self.setVariable("aura", Aura.Hydro);
    };
  };
};

define character {
  id 4206 as TavernAbyssHeraldWickedTorrents;
  tags hydro, monster;
  health 6;
  energy 2;
  skills RipplingSlash,
    VortexEdge,
    TorrentialShock,
    WateryRebirth,
    RipplingBlades,
    TorrentialWard,
    WateryRebirth01;
};
