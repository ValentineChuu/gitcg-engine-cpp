"""Explicit preconditions for the branches left after stage games and generic probes."""
import json
import pathlib

cases = []
def add(definition, name, **options):
    cases.append({'definition': definition, 'name': name, 'health': [[10, 10, 10], [10, 10, 10]], **options})

add(212, 'increase Moondrift effect to three', cards=[211], variables={'effect': 2}, prefer=[211])
add(205, 'append Thundercloud by lunar electrocharged', characters=[1418, 1217, 1101],
    cards=[332012], auras=[[0, 0, 0], [2, 2, 2]], prefer=[332012, 14182])
add(303082, 'blessing sees dendro aura after physical skill', equipped=True, characters=[1101, 1201, 1301],
    auras=[[0, 0, 0], [7, 7, 7]], prefer=[11011])
for character in [2102, 2701, 1213, 1209, 2305, 4103, 4104]:
    add(character, 'revive passive', talent=False, activeIndices=[1, 0],
        health=[[0, 10, 10], [10, 10, 10]], characterVariables=[[{'alive': 0}, {}, {}], [{}, {}, {}]],
        cards=[333009], prefer=[333009])
add(211161, 'discard Void Rift with Dockhands Assistant', equipped=True,
    cards=[111163], entities=[{'definition': 311509}], turn=1, prefer=[13011])
add(127032, 'switch into spirit serpent with Ringdancer talent', equipped=True,
    characters=[2703, 1101, 1301], activeIndices=[1, 0], switchTo=2703,
    entities=[{'definition': 227031}])
for definition in [214022, 226011]:
    add(definition, 'force talent staged action', talent=False, prefer=[definition], health=[[10, 10, 10], [1, 1, 1]])
add(226011, 'talented Lawachurl defeats opponent', equipped=True, prefer=[26013, 26012, 26011],
    health=[[10, 10, 10], [1, 1, 1]])
add(216117, 'electro damage meets electro resistance reduction', turn=1, opponents=[1403, 1301, 1201], prefer=[14032])
add(212151, 'Dahlia talent saves a character with Favonian Favor', equipped=True,
    health=[[1, 10, 10], [10, 10, 10]], entities=[{'definition': 112151}], turn=1, prefer=[13011])
add(122051, 'overload switches away during exhausted bubble preparation', equipped=True,
    variables={'usage': 0}, entities=[{'definition': 122053}], auras=[[3, 0, 0], [0, 0, 0]],
    opponents=[1403, 1301, 1201], turn=1, prefer=[14032])
add(222062, 'Tulpa takes hydro damage', turn=1, opponents=[1201, 1301, 1101], prefer=[12011])
add(113135, 'opponent suffers overloaded', characters=[1313, 1101, 1201], auras=[[0, 0, 0], [4, 0, 0]], prefer=[13132])
for character, opponent, skill in [(3411, 1412, 14121), (3010, 1412, 14121), (3408, 1110, 11101),
                                    (3308, 1110, 11101), (3307, 1412, 14121), (3704, 1412, 14121)]:
    add(character, 'damage from the required Arkhe-tagged character', turn=1,
        opponents=[opponent, 1301, 1201], prefer=[skill])
add(137013, 'winged shroom defeated', turn=1, health=[[1, 10, 10], [10, 10, 10]], prefer=[13011])
for status, character, opponent, skill, own_skill in [(136022, 3602, 1601, 16011, 36022), (134091, 3409, 1403, 14032, 34092)]:
    for level in [0, 2]:
        add(status, f'rifthound level {level} and matching elemental damage', characters=[character, 1101, 1301],
            variables={'level': level}, turn=1, opponents=[opponent, 1301, 1201], prefer=[skill, own_skill])
add(3209, 'hydro phantasm freezes', turn=1, opponents=[1101, 1301, 1201], auras=[[2, 0, 0], [0, 0, 0]], prefer=[11012])
add(136052, 'Stone Drake protects its master', characters=[3606, 1101, 1201],
    entities=[{'definition': 136051}], turn=1, prefer=[13012])
for status in [121023, 121024]:
    add(status, 'Signora prevents lethal damage', phaseStart=False, turn=1,
        health=[[1, 10, 10], [10, 10, 10]], prefer=[13011])
add(121023, 'Signora uses Carmine Chrysalis before transformation', phaseStart=False, prefer=[21023])
for status, opponent, skill in [(130051, 1101, 11012), (130053, 1301, 13012), (130055, 1501, 15011), (130057, 1701, 17012)]:
    add(status, 'Ruin Drake halves matching damage', turn=1, opponents=[opponent, 1201, 1301], prefer=[skill])
add(135021, 'Anemoboxer absorbs pyro then counters', turn=1, prefer=[13012])
add(135021, 'overload switches away from Stormwind Barrier', turn=1, opponents=[1403, 1301, 1201],
    auras=[[3, 0, 0], [0, 0, 0]], prefer=[14032])
output = pathlib.Path(__file__).resolve().parent / 'generated/coverage_focused_cases.json'
output.write_text(json.dumps(cases, indent=2) + '\n')
print(f'{len(cases)} focused cases -> {output}')
