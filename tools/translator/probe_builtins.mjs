// Independent Node oracle for executable VM built-ins, using the same source attribute fixture as C++.
import {readFileSync} from 'node:fs';
import {pathToFileURL} from 'node:url';
import path from 'node:path';
import {root} from './compiler.mjs';
const coreRoot=path.join(root,'reference/piovium/packages/core/dist');
const {Registry,customEvent}=await import(pathToFileURL(path.join(coreRoot,'data.js')));
const {createDefine}=await import(pathToFileURL(path.join(coreRoot,'gts/runtime.js')));
const {default:rootVm}=await import(pathToFileURL(path.join(coreRoot,'gts/vm.js')));
const {Game,StateSymbol,createOfficialVersionResolver}=await import(pathToFileURL(path.join(coreRoot,'index.js')));
const fixture=JSON.parse(readFileSync(new URL('./fixtures/attributes.json',import.meta.url),'utf8'));
const events=new Map();
const functions={701:()=>{},702:()=>{},703:(state,self,extension)=>extension.counts[0]+2.1256789,704:()=>42,705:(state,event)=>{state.counts[event.who]++;}};
function value(v) {
  if(v && typeof v==='object' && v.$lambda)return functions[v.$lambda]??(()=>{throw Error(`lambda slot ${v.$lambda} not translated`);});
  if(v?.$event) {const key=v.identity??v.$event;if(!events.has(key))events.set(key,customEvent(v.$event));return events.get(key);}
  return v;
}
function attribute(v) {return {name:v.name,positionals:()=>v.positionals.map(value),named:v.attributes.length?{attributes:v.attributes.map(attribute)}:null};}
const registry=new Registry(),scope=registry.begin();
for(const definition of fixture)createDefine(rootVm,attribute(definition));
scope.end();const data=registry.resolve(createOfficialVersionResolver('v7.1.0'));
function initial() {
  const deck={characters:[9001,9002],cards:[],noShuffle:true};
  const state=Game.createInitialState({data,decks:[deck,deck],randomSeed:1});
  state.phase='action';state.roundNumber=1;
  for(const player of state.players)player.activeCharacterId=player.characters[0].id;
  return state;
}
function add(state,definitionId,area='characters',masterIndex=0) {
  const definition=data.entities.get(definitionId),player=state.players[0];
  const entity={[StateSymbol]:'entity',id:state.iterators.id--,definition,variables:Object.fromEntries(Object.entries(definition.varConfigs).map(([k,v])=>[k,v.initialValue])),attachments:[]};
  if(area==='characters')player.characters[masterIndex].entities.push(entity);else player[area].push(entity);
  return entity;
}
function info(caller,definition) {return {caller,definition,requestBy:null,charged:false,plunging:false,prepared:false,environment:'normal',finalizeMode:'simple'};}
function action(state,caller,skill,event={onTimeState:state}) {
  const [next,result]=skill.action(state,info(caller,skill),event);if(result.error)throw result.error;
  return [next,result];
}
const result={};
{
  const state=initial(),entity=add(state,990001),skill=entity.definition.skills.find(s=>s.triggerOn==='modifyDamage3');
  const target=state.players[0].characters[0];
  let current=3;
  const event={onTimeState:state,type:3,get value(){return current;},target:{...target,isActive:()=>true},decreaseDamage(n){current-=n;}};
  result.shieldFilter=skill.filter(state,info(entity,skill),event);
  const [next]=action(state,entity,skill,event);
  result.shield={damage:current,remaining:next.players[0].characters[0].entities.length,removed:next.players[0].removedEntities.length};
}
{
  const state=initial(),entity=add(state,990002),skill=entity.definition.skills.find(s=>s.triggerOn==='onEndPhase');
  const [used]=action(state,entity,skill);
  result.usageAfter=used.players[0].characters[0].entities[0].variables.usagePerRound;
  const after=used.players[0].characters[0].entities[0],round=entity.definition.skills.find(s=>s.triggerOn==='onRoundEnd');
  const [next]=action(used,after,round,{onTimeState:used});
  result.roundReset={...next.players[0].characters[0].entities[0].variables};
  const [expired]=action(next,next.players[0].characters[0].entities[0],round,{onTimeState:next});
  result.expired=expired.players[0].characters[0].entities.length===0;
}
{
  const state=initial(),entity=add(state,990003),skill=entity.definition.skills.find(s=>s.triggerOn==='replaceAction');
  const [next,outcome]=action(state,entity,skill,{onTimeState:state,who:0});
  result.prepare={remaining:next.players[0].characters[0].entities.map(e=>({id:e.definition.id,variables:e.variables})),events:outcome.emittedEvents.map(([name])=>name)};
}
{
  const state=initial(),old=add(state,990004,'characters',1),entity=add(state,990008),skill=entity.definition.skills.find(s=>s.triggerOn==='onEnter');
  const [next]=action(state,entity,skill,{onTimeState:state,entity});
  result.conflict=next.players[0].characters.map(c=>c.entities.map(e=>e.definition.id));
}
{
  const state=initial(),entity=add(state,990007),skill=entity.definition.skills.find(s=>s.triggerOn==='onBeforeAction');
  result.nightsoulFilter=skill.filter(state,info(entity,skill),{onTimeState:state,who:1});
  const [next]=action(state,entity,skill,{onTimeState:state,who:1});
  result.nightsoulDisposed=next.players[0].characters[0].entities.length===0;
}
{
  const state=initial(),entity=add(state,991001,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  result.artifactTargets=skill.initiativeSkillConfig.getTarget(state,info(entity,skill)).map(arg=>arg.targets.map(t=>t.definition.id));
  const [next]=action(state,entity,skill,{targets:[state.players[0].characters[1]]});
  result.equipment={hands:next.players[0].hands.length,equipped:next.players[0].characters[1].entities[0].definition.id};
}
{
  const state=initial(),entity=add(state,991002,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  const [next]=action(state,entity,skill,{targets:[]});
  result.support={hands:next.players[0].hands.length,supports:next.players[0].supports.map(s=>s.definition.id)};
}
{
  const state=initial(),entity=add(state,991003,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  result.foodTargets=skill.initiativeSkillConfig.getTarget(state,info(entity,skill)).map(arg=>arg.targets.map(t=>t.definition.id));
}
{
  const state=initial(),entity=add(state,991001,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  state.players[0].activeCharacterId=state.players[0].characters[1].id;
  result.rotatedTargets=skill.initiativeSkillConfig.getTarget(state,info(entity,skill)).map(arg=>arg.targets.map(t=>t.definition.id));
}
{
  const state=initial(),entity=add(state,990010),skill=entity.definition.skills.find(s=>s.triggerOn==='onCustomEvent');
  const event={onTimeState:state,entity,customEvent:events.get('fixture')};
  result.customFilter=skill.filter(state,info(entity,skill),event);
  result.customOther=skill.filter(state,info(entity,skill),{...event,customEvent:customEvent('test')});
  const [,outcome]=skill.action(state,info(entity,skill),event);
  result.unbound=outcome.error?.message??null;
}
{
  const state=initial(),skill=data.extensions.values().next().value.skills[0];
  const [next]=action(state,state.players[0].characters[0],skill,{onTimeState:state,who:1});
  result.extension=next.extensions[0].state;
}
{
  const state=initial(),entity=add(state,991004),skill=entity.definition.skills.find(s=>s.skillType==='technique');
  const [next]=action(state,entity,skill,{targets:[]});
  result.technique=next.players[0].characters[0].entities[0].variables;
}
{
  const state=initial();
  result.attachment=data.attachments.get(999001).modifications(state,0);
}
{
  const state=initial(),entity=add(state,990005,'summons'),skill=entity.definition.skills.find(s=>s.triggerOn==='onDamageOrHeal');
  const source=state.players[0].characters[0],target=state.players[1].characters[0];
  const event={onTimeState:state,source,target,type:5,isDamageTypeDamage:()=>true,isSwirl:()=>3};
  result.swirlFilter=skill.filter(state,info(entity,skill),event);
  const [next]=action(state,entity,skill,event);
  result.swirlHint=next.players[0].summons[0].variables;
}
{
  const state=initial(),entity=add(state,991003,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  const [next]=action(state,entity,skill,{targets:[state.players[0].characters[0]]});
  result.foodSatiated=next.players[0].characters[0].entities.map(e=>e.definition.id);
}
{
  const state=initial(),blessing=add(state,990007),entity=add(state,991004),skill=entity.definition.skills.find(s=>s.triggerOn==='onBeforeAction');
  const [next,outcome]=action(state,entity,skill,{onTimeState:state,who:1});
  const dispose=entity.definition.skills.find(s=>s.triggerOn==='onDispose');
  const [,event]=outcome.emittedEvents.find(([name])=>name==='onDispose');
  const [final]=action(next,entity,dispose,event);
  result.techniqueNightsoul=final.players[0].characters[0].entities.map(e=>e.definition.id);
}
{
  const state=initial(),entity=add(state,991005,'pile');add(state,991005,'hands');
  state.players[0].dice=[2,2,2,2];
  const skill=entity.definition.skills.find(s=>s.triggerOn==='onActionPhase');
  result.blessingFilter=skill.filter(state,info(entity,skill),{onTimeState:state});
  const [next]=action(state,entity,skill,{onTimeState:state});
  result.blessing={hands:next.players[0].hands.length,pile:next.players[0].pile.length,supports:next.players[0].supports.map(s=>s.definition.id),dice:next.players[0].dice};
}
{
  const state=initial(),entity=add(state,991006,'hands'),skill=entity.definition.skills.find(s=>s.triggerOn==='initiative');
  const [next,outcome]=action(state,entity,skill,{targets:[]});
  result.adventure={variables:next.players[0].supports[0].variables,events:outcome.emittedEvents.map(([name])=>name)};
}
{
  const state=initial(),entity=add(state,990012,'summons');
  result.description={hint:entity.definition.descriptionDictionary['[GCG_TOKEN_HINT_TEXT]'](state,entity.id),text:entity.definition.descriptionDictionary.X(state,entity.id)};
}
console.log(JSON.stringify(result));
