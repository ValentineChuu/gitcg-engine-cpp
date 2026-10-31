// Stun (100001): the generic "cannot use Skills until the end of this Round" status.
// Applied when a Ward or Elemental Equipment status runs out of element instances
// (the wiki calls this "Stun" on Hydro Elemental Equipment).

/**
 * @id 100001
 * @name Stun
 * @description
 * Character cannot use Skills. (Lasts until the end of this Round)
 */
define status {
  id 100001 as Stun;
  oneDuration;
  tags disableSkill;
};
