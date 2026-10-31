// Tallies every elemental reaction (and what caused it) and all damage per player over a whole game.
// A reaction's cause is "<caller type>|<skill type>": e.g. "character|elemental" for an Elemental Skill,
// "summon|-" for a summon's attack, "status|-" or "combatStatus|-" for triggered effects.
// The core keeps per-phase damage/reaction logs in state and clears them often; our core patch calls
// game.beforeClearPhaseLogs(state) just before each clear, and summary() reads whatever is left at the end.

export class CombatLog {
  constructor(game) {
    this.seen = new Set();
    this.reactions = [{}, {}]; // per player: reaction type code (101 Melt, 102 Vaporize, ...) -> count
    this.reactionCauses = [{}, {}]; // per player: "<reaction code>|<caller type>|<skill type>" -> count
    this.damageDealt = [0, 0];
    game.beforeClearPhaseLogs = (state) => this.scan(state);
  }

  scan(state) {
    state.players.forEach((player, who) => {
      for (const event of player.phaseReactionLog) {
        if (this.seen.has(event)) continue;
        this.seen.add(event);
        const { type, via } = event.reactionInfo;
        this.reactions[who][type] = (this.reactions[who][type] ?? 0) + 1;
        const cause = `${type}|${via?.caller?.definition?.type ?? "unknown"}|${via?.definition?.skillType ?? "-"}`;
        this.reactionCauses[who][cause] = (this.reactionCauses[who][cause] ?? 0) + 1;
      }
      for (const event of player.phaseDamageLog) {
        if (this.seen.has(event)) continue;
        this.seen.add(event);
        if (!event.isDamageTypeHeal()) this.damageDealt[who] += event.value;
      }
    });
  }

  summary(state) {
    this.scan(state);
    return { reactions: this.reactions, reactionCauses: this.reactionCauses, damageDealt: this.damageDealt };
  }
}
