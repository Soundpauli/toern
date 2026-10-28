const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../handbook/handbook-search.js'), 'utf8');
const storage = new Map();
const chapter = {href:'subpatterns.html', title:'Subpatterns', desc:'A–E file slots, SUBP', group:'Compose'};
const root = {textContent:'Five variations with notes and pages.', querySelectorAll(){return [];}};
const parsed = {
 body:root,
 querySelector(selector){
  if(selector.includes('.wrap'))return root;
  if(selector==='.handbook-page__title')return {textContent:'Five possible futures'};
  if(selector==='.handbook-page__summary')return {textContent:'Alternate musical arrangements.'};
  return null;
 }
};
const context = vm.createContext({
 window:{HBHandbook:{pages:[chapter]}},
 document:{readyState:'loading',addEventListener(){}},
 DOMParser:class {parseFromString(){return parsed;}},
 sessionStorage:{getItem:k=>storage.get(k),setItem:(k,v)=>storage.set(k,v)},
 fetch:async()=>({ok:true,text:async()=>'<html></html>'})
});
vm.runInContext(source.replace('  if (document.readyState === "loading") {',
 '  window.testSearch = {extractDoc, searchDocs, loadCached, saveCached, buildIndex, ensureIndex, highlight};\n  if (document.readyState === "loading") {'), context);
const api=context.window.testSearch;
(async()=>{
 const doc=api.extractDoc('',chapter);
 // Canonical chapter names remain searchable even when translated copy omits them.
 for(const query of ['subpattern','subpat','pattern','PATTERNS','subp']){
  const results=api.searchDocs([doc],query);
  assert.equal(results.length,1,query);
  assert.equal(results[0].href,'subpatterns.html');
 }
 assert.equal(api.searchDocs([doc],'subpattern nonexistent').length,0);
 assert.equal(api.highlight('Subpatterns',['subpattern']),'<mark>Subpattern</mark>s');
 api.saveCached([doc]);assert.equal(api.loadCached().length,1);
 context.window.HBHandbook.pages.push({href:'new.html',title:'New chapter'});
 assert.equal(api.loadCached(),null,'chapter-list changes invalidate cached search');
 context.window.HBHandbook.pages.pop();storage.clear();
 context.fetch=async()=>{throw Error('temporarily unavailable');};
 const fallback=await api.buildIndex();
 assert.equal(api.searchDocs(fallback,'subpattern').length,1);
 assert.equal(storage.size,0,'partial indexes must not persist');
 context.fetch=async()=>({ok:true,text:async()=>'<html></html>'});
 const rebuilt=await api.ensureIndex();
 assert.equal(rebuilt[0].body,'five variations with notes and pages.');
 assert.equal(api.loadCached().length,1);
 console.log('PASS: partial words, canonical chapter names, cache invalidation, and fetch retry.');
})().catch(error=>{console.error(error);process.exitCode=1;});
