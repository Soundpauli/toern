import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { addEncoderAssemblies } from './encoder-assemblies.js';

const $ = (id) => document.getElementById(id);
const embedded = document.body.classList.contains('embedded');
const esc = (s) => String(s ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const viewport = $('viewport'), canvas = $('scene');
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(36, 1, 4, 800);
camera.position.set(0, 190, 155);
const renderer = new THREE.WebGLRenderer({canvas, antialias:true, alpha:true});
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
renderer.setClearColor(0x000000, 0);
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.NeutralToneMapping;
renderer.toneMappingExposure = 1;
const environmentGenerator = new THREE.PMREMGenerator(renderer);
const studio = new RoomEnvironment();
const environment = environmentGenerator.fromScene(studio, .04);
scene.environment = environment.texture;
scene.environmentIntensity = .65;
studio.dispose();
environmentGenerator.dispose();
const controls = new OrbitControls(camera, canvas);
controls.enableDamping = true;
if(embedded){
controls.enableZoom = false;
canvas.addEventListener('wheel',event=>{controls.enableZoom=event.ctrlKey||event.metaKey;},{capture:true,passive:true});
canvas.addEventListener('touchstart',event=>{controls.enableZoom=event.touches.length>1;},{passive:true});
}
controls.dampingFactor = .09;
controls.minDistance = 6;
controls.maxDistance = 650;
controls.target.set(0, 2, 0);
scene.add(new THREE.HemisphereLight(0xffffff, 0x777777, .55));
for (const [pos, intensity] of [[[20,180,60],2.2], [[-150,70,-100],.65], [[80,-120,50],.7]]) {
 const light = new THREE.DirectionalLight(0xffffff, intensity);
 light.position.set(...pos); scene.add(light);
}
const modelGroup = new THREE.Group();
modelGroup.scale.setScalar(1000);
scene.add(modelGroup);
const raycaster = new THREE.Raycaster(), mouse = new THREE.Vector2();
const objects = new Map(), boxes = new Map(), labelNodes = new Map();
const pickMeshes = [];
const selectionOutline = new THREE.Box3Helper(new THREE.Box3(), 0xff3b30);
selectionOutline.visible = false; selectionOutline.material.depthTest=false;
selectionOutline.renderOrder = 20; scene.add(selectionOutline);
const pinGroup = new THREE.Group(); scene.add(pinGroup);
let data, parts=[], byRef, boardRoot, selected, selectedPin=null, category='Connectors', query='';
let origin = new THREE.Vector3(), animation=null, pointerDown=null, currentView='iso';
let moduleVisible=false, ready=false;
let encodersVisible=true, encoderGroup, setEncoderColor;
const pcbMaterials=[];
function setPcbColor(value){
 const mask=value==='black'?0x151b18:0x0b512f;
 const edge=value==='black'?0x101412:0x30452b;
 for(const [material,isEdge] of pcbMaterials)material.color.set(isEdge?edge:mask);
 setEncoderColor?.(mask);
}
const dotMaterial = new THREE.MeshBasicMaterial({color:0xff3b30,depthTest:false});
const peerMaterial = new THREE.MeshBasicMaterial({color:0xff3b30,depthTest:false});
const guideMaterial = new THREE.LineBasicMaterial({color:0xff3b30,transparent:true,opacity:.55,depthTest:false});
const point = (p, height=3.2) => new THREE.Vector3(p.x-origin.x, (p.height??height)-origin.y, p.z-origin.z);
const sanitize = THREE.PropertyBinding.sanitizeNodeName;
const categoryMatches = (p) => category==='All' || (category==='Connectors' ? ['I/O','JST','Audio'].includes(p.category) && p.ref!=='MK1' : p.category===category);
const queryMatches = (p) => !query || `${p.ref} ${p.title} ${p.value} ${p.footprint} ${p.description} ${p.pads.map(x=>x.net).join(' ')}`.toLowerCase().includes(query);
const isMatch = p => categoryMatches(p) && queryMatches(p);

function finishMaterials(root){
 // These jack STEP files contain unpainted solids (or one blue CAD color),
 // rather than the black molded housing finish of the fitted connectors.
 for(const ref of ['J1','J3','J5','J6','J7','J17']){
  const copies=new Map();
  root.getObjectByName(sanitize(ref))?.traverse(mesh=>{if(!mesh.isMesh)return;
   const finish=m=>{if(copies.has(m))return copies.get(m);const material=m.clone();material.name='jack-housing';material.color.set(0x363a3e);copies.set(m,material);return material;};
   mesh.material=Array.isArray(mesh.material)?mesh.material.map(finish):finish(mesh.material);
  });
 }
 const moduleCopies=new Map();
 root.getObjectByName(sanitize('U6'))?.traverse(mesh=>{if(!mesh.isMesh)return;
  const finish=m=>{if(m.name)return m;if(moduleCopies.has(m))return moduleCopies.get(m);const material=m.clone();material.color.set(0x7a8083);moduleCopies.set(m,material);return material;};
  mesh.material=Array.isArray(mesh.material)?mesh.material.map(finish):finish(mesh.material);
 });
 const finished=new Set();
 // STEP exports omit metallicFactor on plastics; glTF's default of 1 makes
 // them behave like metal. Use a dielectric finish and separate actual metals.
 const metalNames=new Set([6,8,10,14,18,20,21,23,24,28,29,32,36,37].map(i=>`mat_${i}`));
 root.traverse(mesh=>{if(!mesh.isMesh)return;
  for(const material of Array.isArray(mesh.material)?mesh.material:[mesh.material]){
   if(finished.has(material))continue;finished.add(material);
   const color=material.color;
   if(material.name==='mat_39'||material.name==='mat_40'){
    pcbMaterials.push([material,false]);
    color.set(0x0b512f);material.metalness=0;material.roughness=.58;
    material.opacity=1;material.transparent=false;material.depthWrite=true;
   }else if(material.name==='mat_41'){
    pcbMaterials.push([material,true]);
    color.set(0x30452b);material.metalness=0;material.roughness=.9;
   }else if(metalNames.has(material.name)){
    material.metalness=.82;material.roughness=.32;
   }else{
    material.metalness=0;material.roughness=.72;
    if(Math.max(color.r,color.g,color.b)<.15){
     color.set(0x363a3e);material.roughness=.86;
     // Fine molded-plastic grain in world millimetres, without requiring UVs.
     material.onBeforeCompile=shader=>{
      shader.vertexShader='varying vec3 vFinishPosition;\n'+shader.vertexShader;
      shader.vertexShader=shader.vertexShader.replace('#include <project_vertex>','#include <project_vertex>\nvFinishPosition = (modelMatrix * vec4(transformed, 1.0)).xyz;');
      shader.fragmentShader='varying vec3 vFinishPosition;\n'+shader.fragmentShader;
      shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',`#include <color_fragment>
       vec3 grainCell = floor(vFinishPosition * 10.0);
       float plasticGrain = fract(sin(dot(grainCell, vec3(12.9898,78.233,37.719))) * 43758.5453);
       float grainFootprint = 10.0 * max(length(dFdx(vFinishPosition)), length(dFdy(vFinishPosition)));
       float grainVisibility = 1.0 - smoothstep(0.4, 1.5, grainFootprint);
       diffuseColor.rgb *= 1.0 + 0.20 * (plasticGrain - 0.5) * grainVisibility;`);
     };
     material.customProgramCacheKey=()=> 'charcoal-plastic-grain-v2-filtered';
    }
   }
   material.needsUpdate=true;
  }
 });
}

function makeFilters(){
 $('filters').innerHTML=['Connectors','JST','Audio','Encoders','Core','Power','Passives','All'].map(x=>`<button aria-pressed="${category===x}" data-category="${x}">${x==='Connectors'?'I/O':x}</button>`).join('');
 for(const b of $('filters').querySelectorAll('button')) b.onclick=()=>{category=b.dataset.category; makeFilters();renderList();};
}
function renderList(){
 const visible=parts.filter(isMatch);
 $('count').textContent=visible.length;
 $('list-title').textContent=category==='Connectors'?'I/O & connectors':category==='All'?'All components':category;
 $('part-list').innerHTML=visible.length ? visible.map(p=>`<button class="part-row" data-ref="${esc(p.ref)}" aria-pressed="${selected?.ref===p.ref}"><span class="ref">${esc(p.ref)}</span><span><span class="part-title">${esc(p.title)}</span><span class="sub">${esc(p.category==='JST'?`JST PH · ${p.pads.length} pin · 2.00 mm`:p.category==='I/O'?'Interface':p.value)}</span></span></button>`).join('') : '<p class="empty">No matching parts or signals.</p>';
 for(const b of $('part-list').querySelectorAll('button')) b.onclick=()=>select(b.dataset.ref);
}
function updateGeometryNotice(){
 const notice=$('geometry-notice'); if(!notice || !selected)return;
 if(!ready){notice.hidden=true;return;}
 notice.textContent=objects.has(selected.ref)?'':'No component solid was exported for this footprint. Selection uses its PCB position and pad extent.';
 notice.hidden=objects.has(selected.ref);
}
function renderInspector(){
 const p=selected;
 $('inspector').innerHTML=`<div class="eyebrow">COMPONENT INSPECTOR</div><div class="component-ref">${esc(p.ref)}</div><h2>${esc(p.title)}</h2><span class="tag">${esc(p.category)}</span>${p.category==='JST'?`<span class="tag">PH · 2.00 mm · ${p.pads.length} pin</span>`:''}<p class="description">${esc(p.description || `${p.value}. Select a pin to inspect its electrical connections.`)}</p>${p.caution?`<div class="caution">${esc(p.caution)}</div>`:''}<div id="geometry-notice" class="model-warning" hidden></div><h3 class="section-title">${p.pads.length?'Pin connections':'Footprint'}</h3>${p.pads.length?'<p class="pin-note">Select a pad to mark it in 3D and see parts on the same net. Pad numbers follow KiCad, viewed on the board.</p>':''}<div class="pins">${p.pads.map((pad,i)=>`<button class="pin-row ${pad.net.startsWith('unconnected-')?'nc':''} ${['GND','+5V','+3.3V','+3.3VA','VBAT','VBUS','5V-HOST'].includes(pad.net)?'power':''}" data-pin="${i}" aria-pressed="false"><span class="pin-number">${esc(pad.number)}</span><span class="pin-name">${pad.label?esc(pad.label)+'<br>':''}${esc(pad.net.startsWith('unconnected-')?'Not connected':pad.net||'No assigned net')}</span></button>`).join('')}</div><div id="pin-detail" class="pin-detail" aria-live="polite"></div><h3 class="section-title">Board data</h3><dl class="facts"><dt>Value</dt><dd>${esc(p.value)}</dd><dt>Footprint</dt><dd>${esc(p.footprint)}</dd><dt>Position</dt><dd>${p.x.toFixed(2)}, ${p.z.toFixed(2)} mm</dd><dt>Rotation</dt><dd>${p.angle}° · ${esc(p.side)}</dd>${p.lcsc?`<dt>LCSC</dt><dd>${esc(p.lcsc)}</dd>`:''}</dl><div class="sources">Source: current <code>toern_revJ.kicad_pcb</code>, board silkscreen and revJ README. GPIO aliases describe net names; active functions depend on firmware. Connection guides show net membership, not copper routing.<br>PCB fingerprint: ${esc(data.sha256.slice(0,12))}<br>Viewer: <a href="https://threejs.org/docs/pages/GLTFLoader.html" target="_blank" rel="noopener">Three.js GLTFLoader</a></div>`;
 for(const b of $('inspector').querySelectorAll('[data-pin]')) b.onclick=()=>selectPin(Number(b.dataset.pin));
 if(embedded){
 const pickerLabel=document.createElement('label');pickerLabel.className='component-picker-label';pickerLabel.textContent='Component';
 const picker=document.createElement('select');picker.id='component-picker';picker.setAttribute('aria-label','Select a board component');
 const categories=[...new Set(parts.map(part=>part.category))];
 for(const category of categories){
  const group=document.createElement('optgroup');group.label=category;
  for(const part of parts.filter(part=>part.category===category)){
   const option=document.createElement('option');option.value=part.ref;option.textContent=`${part.ref} · ${part.title}`;option.selected=part.ref===p.ref;group.append(option);
  }
  picker.append(group);
 }
 picker.onchange=()=>select(picker.value);pickerLabel.append(picker);$('inspector').querySelector('.eyebrow').after(pickerLabel);

 }
 if(p.category==='Encoders'){
  const source=document.createElement('div');source.className='sources';
  source.innerHTML='Encoder models: DuPPa / Simone Caron, upstream revision d47fd5a. <a href="./assets/encoder-NOTICE.txt" target="_blank">Attribution and model notes</a> · <a href="./assets/encoder-LICENSE.txt" target="_blank">GPLv3</a>.';
  $('inspector').append(source);
 }
 updateGeometryNotice();
}
function select(ref){
 const p=byRef.get(ref); if(!p)return;
 selected=p;selectedPin=null;clearPins();renderList();renderInspector();
 $('selection-status').textContent=`${p.ref} / ${p.title}`;
 updateSelection();
 history.replaceState(null,'',`#${encodeURIComponent(p.ref)}`);
}
function clearPins(){
 for(const child of [...pinGroup.children]){pinGroup.remove(child);child.geometry?.dispose();}
}
function selectPin(index){
 selectedPin=selected.pads[index];clearPins();
 for(const b of $('inspector').querySelectorAll('[data-pin]')){const active=Number(b.dataset.pin)===index;b.classList.toggle('active',active);b.setAttribute('aria-pressed',active);}
 const pad=selectedPin;
 const endpoints=pad.net.startsWith('unconnected-')||!pad.net?[]:pad.peers.filter(x=>x.ref!==selected.ref);
 const unique=[...new Set(endpoints.map(x=>x.ref))];
 $('pin-detail').innerHTML=`<p><strong>Pad ${esc(pad.number)}</strong> · ${esc(pad.net||'No net')}<br>${unique.length?`${unique.length} other component${unique.length===1?'':'s'} on this net.`:'No other components on this net.'}</p><div class="peer-list">${unique.map(ref=>`<button data-peer="${esc(ref)}">${esc(ref)}</button>`).join('')}</div>${unique.length?'<p style="margin-top:10px">Straight guides show electrical connections, not physical traces.</p>':''}`;
 for(const b of $('pin-detail').querySelectorAll('[data-peer]'))b.onclick=()=>select(b.dataset.peer);
 const pos=point(pad,3.8), marker=new THREE.Mesh(new THREE.SphereGeometry(.85,14,10),dotMaterial);
 marker.position.copy(pos);marker.renderOrder=30;pinGroup.add(marker);
 for(const ref of unique.slice(0,80)){
  const peer=byRef.get(ref);if(!peer)continue;
  const peerPad=peer.pads.find(x=>x.net===pad.net),end=point(peerPad||peer,4);
  const dot=new THREE.Mesh(new THREE.SphereGeometry(.4,8,6),peerMaterial);dot.position.copy(end);dot.renderOrder=25;pinGroup.add(dot);
  const line=new THREE.Line(new THREE.BufferGeometry().setFromPoints([pos,end]),guideMaterial);line.renderOrder=21;pinGroup.add(line);
 }
}
function updateSelection(){
 if(!selected || !ready)return;
 if((selected.ref.startsWith('ENC')&&!encodersVisible)||(selected.ref==='U6'&&!moduleVisible)){selectionOutline.visible=false;return;}
 const box=boxes.get(selected.ref);
 if(box){selectionOutline.box.copy(box).expandByScalar(.5);selectionOutline.visible=true;}
}
function fit(view='iso',focus=false){
 currentView=view;
 const box=focus&&selected?boxes.get(selected.ref):new THREE.Box3().setFromObject(modelGroup);
 if(!focus&&encoderGroup?.visible)box.union(new THREE.Box3().setFromObject(encoderGroup));
 if(!box)return;
 const target=box.getCenter(new THREE.Vector3()),size=box.getSize(new THREE.Vector3());
 const dim=Math.max(size.x, size.z, size.y, focus?14:80);
 let distance=(dim/(2*Math.tan(THREE.MathUtils.degToRad(camera.fov/2)))) / Math.min(camera.aspect,1) * (focus?1.55:1.15);
 const direction=view==='top'?new THREE.Vector3(0,1,.0001):view==='bottom'?new THREE.Vector3(0,-1,.0001):new THREE.Vector3(.12,1,.83).normalize();
 if(embedded){
  // Fit the projected bounds to both axes, using the full website canvas
  // rather than reserving the standalone explorer's sidebar-sized space.
  const right=new THREE.Vector3().crossVectors(camera.up,direction).normalize();
  const up=new THREE.Vector3().crossVectors(direction,right).normalize();
  const tangent=Math.tan(THREE.MathUtils.degToRad(camera.fov/2));
  distance=6;
  for(const x of [box.min.x,box.max.x])for(const y of [box.min.y,box.max.y])for(const z of [box.min.z,box.max.z]){
   const offset=new THREE.Vector3(x,y,z).sub(target),depth=offset.dot(direction);
   distance=Math.max(distance,depth+Math.abs(offset.dot(right))/(tangent*camera.aspect),depth+Math.abs(offset.dot(up))/tangent);
  }
  distance*=focus?1.4:1.12;
 }
  animation={start:performance.now(),fromPosition:camera.position.clone(),fromTarget:controls.target.clone(),toPosition:target.clone().addScaledVector(direction,distance),toTarget:target};
  if(matchMedia('(prefers-reduced-motion: reduce)').matches)animation.start-=500;
 for(const b of $('views').querySelectorAll('button'))b.setAttribute('aria-pressed',b.dataset.view===view);
}
function attachLabels(){
 for(const p of parts){
  if(labelNodes.has(p.ref))continue;
  const b=document.createElement('button');b.className='part-label';b.dataset.ref=p.ref;b.setAttribute('aria-label',`${p.ref}: ${p.title}`);b.hidden=true;b.onclick=()=>select(p.ref);$('labels').append(b);labelNodes.set(p.ref,b);
 }
}
function updateLabels(){
 const width=viewport.clientWidth,height=viewport.clientHeight;
 const occupied=[];
 const visibleBoxes=[...boxes].filter(([ref])=>(moduleVisible||ref!=='U6')&&(encodersVisible||!ref.startsWith('ENC')));
 const ordered=[...parts].sort((a,b)=>(a.ref===selected?.ref?-1:b.ref===selected?.ref?1:0));
 for(const p of ordered){
  const el=labelNodes.get(p.ref);if(!el)continue;
  const active=p.ref===selected?.ref;
  const canShow=active&&(moduleVisible||p.ref!=='U6')&&(encodersVisible||!p.ref.startsWith('ENC'));
  if(!canShow){el.hidden=true;continue;}
  const box=boxes.get(p.ref);if(!box){el.hidden=true;continue;}
  const anchor=box.getCenter(new THREE.Vector3());anchor.y=camera.position.y>=0?box.max.y+1:box.min.y-1;
  const world=anchor.clone();const projection=anchor.project(camera);
  if(projection.z>1||projection.z< -1){el.hidden=true;continue;}
  let x=(projection.x*.5+.5)*width,y=(-projection.y*.5+.5)*height;
  const text=active || (width>620&&['J33','J4','J16','J10','J25','J26'].includes(p.ref));
  const labelHTML=`${esc(p.ref)}${text?`<span class="label-name">${esc(p.title)}</span>`:''}`;
  if(el.innerHTML!==labelHTML)el.innerHTML=labelHTML;
  el.classList.toggle('active',active);
  const w=el.offsetWidth || (text?170:40),h=26;
  if(active){x=Math.max(w/2+10,Math.min(width-w/2-10,x));y=Math.max(h+20,Math.min(height-10,y));}
  const rect={l:x-w/2-3,r:x+w/2+3,t:y-h-12,b:y+3};
  if(rect.l<5||rect.r>width-5||rect.t<5||rect.b>height-5 || (!active&&occupied.some(r=>!(rect.l>r.r||rect.r<r.l||rect.t>r.b||rect.b<r.t)))){el.hidden=true;continue;}
  // Hide labels for solids blocked by other geometry, including pogo contacts under Teensy.
  if(!active){
   const rayDirection=world.clone().sub(camera.position);const d=rayDirection.length();raycaster.set(camera.position,rayDirection.normalize());
   const intersection=new THREE.Vector3();
   const blocked=visibleBoxes.some(([ref,bounds])=>ref!==p.ref && raycaster.ray.intersectBox(bounds,intersection) && camera.position.distanceTo(intersection)<d-1.2);
   if(blocked){el.hidden=true;continue;}
  }
  occupied.push(rect);el.style.left=`${x}px`;el.style.top=`${y-5}px`;el.hidden=false;
 }
}
function resize(){
 const w=viewport.clientWidth,h=viewport.clientHeight;renderer.setSize(w,h,false);camera.aspect=w/h;camera.updateProjectionMatrix();
}
new ResizeObserver(resize).observe(viewport);
function hitPart(event){
 const r=canvas.getBoundingClientRect();mouse.set((event.clientX-r.left)/r.width*2-1,-(event.clientY-r.top)/r.height*2+1);raycaster.setFromCamera(mouse,camera);
 for(const hit of raycaster.intersectObjects(pickMeshes,false)){
  let o=hit.object,visible=true;while(o){if(!o.visible){visible=false;break;}o=o.parent;}
  if(!visible)continue;
  if(hit.object.userData.ref)return hit.object.userData.ref;
  return null;
 }
 return null;
}
canvas.addEventListener('pointerdown',e=>{pointerDown={x:e.clientX,y:e.clientY};animation=null;});
canvas.addEventListener('pointerup',e=>{if(pointerDown&&Math.hypot(e.clientX-pointerDown.x,e.clientY-pointerDown.y)<5){const ref=hitPart(e);if(ref)select(ref);}pointerDown=null;});
canvas.addEventListener('pointermove',e=>{
 if(e.buttons||!ready){$('hover').hidden=true;return;}
 const ref=hitPart(e),p=byRef.get(ref);canvas.style.cursor=p?'pointer':'grab';
 if(!p){$('hover').hidden=true;return;}
 const rect=viewport.getBoundingClientRect();$('hover').textContent=`${p.ref} · ${p.title}`;$('hover').hidden=false;
 $('hover').style.left=`${Math.min(e.clientX-rect.left+14,rect.width-240)}px`;$('hover').style.top=`${Math.max(4,e.clientY-rect.top-35)}px`;
});
canvas.addEventListener('pointerleave',()=>{$('hover').hidden=true;});
canvas.addEventListener('dblclick',e=>{const ref=hitPart(e);if(ref){select(ref);fit(currentView,true);}});
for(const b of $('views').querySelectorAll('button'))b.onclick=()=>fit(b.dataset.view);
$('fit').onclick=()=>fit();$('focus').onclick=()=>fit(currentView,true);
$('search').oninput=e=>{query=e.target.value.trim().toLowerCase();renderList();};
$('pcb-color').onchange=e=>setPcbColor(e.target.value);
$('module-toggle').onchange=e=>{moduleVisible=e.target.checked;const module=objects.get('U6');if(module)module.visible=moduleVisible;};
$('encoders-toggle').onchange=e=>{encodersVisible=e.target.checked;if(encoderGroup)encoderGroup.visible=encodersVisible;updateSelection();};
window.addEventListener('hashchange',()=>{const ref=decodeURIComponent(location.hash.slice(1));if(byRef?.has(ref))select(ref);});

async function init(){
 const requestedRef=decodeURIComponent(location.hash.slice(1));
 const response=await fetch('./assets/board-data.json');if(!response.ok)throw new Error('Could not load board data');data=await response.json();parts=data.parts;byRef=new Map(parts.map(p=>[p.ref,p]));makeFilters();attachLabels();select(byRef.has(requestedRef)?requestedRef:'J33');
 const [gltf,encoderGLTF,rotaryGLTF]=await Promise.all([
  new GLTFLoader().loadAsync('./assets/revj.glb',event=>{if(event.total)$('loading-text').textContent=`Loading board geometry · ${Math.round(event.loaded/event.total*100)}%`;}),
  new GLTFLoader().loadAsync('./assets/encoder-module.glb'),
  new GLTFLoader().loadAsync('./assets/rgb-encoder.glb')
 ]);
 boardRoot=gltf.scene;
 finishMaterials(boardRoot);
 // KiCad emits one primitive per CAD face. Merge by material within each
 // component to keep orbiting smooth while preserving part selection.
 boardRoot.updateMatrixWorld(true);
 for(const node of [...boardRoot.children[0].children]){
  if(node.isMesh)continue;
  const groups=new Map(),inverse=node.matrixWorld.clone().invert();
  node.traverse(mesh=>{if(!mesh.isMesh)return;
   const material=mesh.material;if(Array.isArray(material))return;
   const geometry=mesh.geometry.clone().applyMatrix4(new THREE.Matrix4().multiplyMatrices(inverse,mesh.matrixWorld));
   if(!groups.has(material))groups.set(material,[]);groups.get(material).push(geometry);
  });
  if(!groups.size)continue;
  const merged=[];
  for(const [material,geometries] of groups){
   const geometry=mergeGeometries(geometries,false);
   if(!geometry){for(const g of geometries)g.dispose();continue;}
   merged.push(new THREE.Mesh(geometry,material));for(const g of geometries)g.dispose();
  }
  if(merged.length===groups.size){node.clear();for(const mesh of merged)node.add(mesh);}
 }
 modelGroup.add(boardRoot);modelGroup.updateMatrixWorld(true);
 const fullBox=new THREE.Box3().setFromObject(modelGroup);
 origin=fullBox.getCenter(new THREE.Vector3());origin.y=0;
 modelGroup.position.copy(origin).multiplyScalar(-1);modelGroup.updateMatrixWorld(true);
 for(const p of parts){
  const obj=boardRoot.getObjectByName(sanitize(p.ref));
  if(obj){objects.set(p.ref,obj);obj.traverse(child=>{if(child.isMesh)child.userData.ref=p.ref;});}
  const box=obj?new THREE.Box3().setFromObject(obj):new THREE.Box3().setFromPoints([point(p,0),...p.pads.map(x=>point(x,2))]).expandByScalar(1);
  boxes.set(p.ref,box);
 }
 boardRoot.traverse(o=>{if(o.isMesh)pickMeshes.push(o);});
 const module=objects.get('U6');if(module)module.visible=moduleVisible;
 const encoders=addEncoderAssemblies({scene,moduleModel:encoderGLTF.scene,rotaryModel:rotaryGLTF.scene,byRef,objects,boxes,pickMeshes,origin});
 encoderGroup=encoders.group;setEncoderColor=encoders.setColor;setPcbColor($('pcb-color').value);encoderGroup.visible=encodersVisible;parts.push(...encoders.parts);attachLabels();renderList();if(byRef.has(requestedRef))select(requestedRef);
 // Missing solids still get a selectable pad-sized volume, invisible in the rendered view.
 for(const p of parts){if(objects.has(p.ref))continue;
  const box=boxes.get(p.ref),size=box.getSize(new THREE.Vector3());
  const proxy=new THREE.Mesh(new THREE.BoxGeometry(Math.max(size.x,2),Math.max(size.y,1),Math.max(size.z,2)),new THREE.MeshBasicMaterial({visible:false}));proxy.position.copy(box.getCenter(new THREE.Vector3()));proxy.userData.ref=p.ref;scene.add(proxy);pickMeshes.push(proxy);
 }
 ready=true;resize();fit();updateSelection();updateGeometryNotice();$('loading').hidden=true;
 // Bounded diagnostics for browser QA; no source data or credentials leave this page.
 window.boardExplorer={getState:()=>({ready,selected:selected?.ref,pin:selectedPin?.number,category,parts:parts.length,models:objects.size,visibleLabels:[...labelNodes.values()].filter(e=>!e.hidden).map(e=>e.dataset.ref),moduleVisible}),select,selectPin,fit};
}
let lastLabels=0;
let canvasVisible=true;
new IntersectionObserver(([entry])=>{canvasVisible=entry.isIntersecting;}).observe(canvas);
function updateCameraDepth(){
 // All coordinates are mm. CAD coating layers are only 0.025–0.05 mm
 // apart; a fixed 0.1 mm near plane loses their separation at wide zoom.
 // Keep the near plane at 2% of the orbit distance, leaving ample room
 // for close inspection while retaining precision at the 650 mm limit.
 const distance=camera.position.distanceTo(controls.target);
 const near=Math.max(.25,distance/50),far=Math.max(400,distance+400);
 if(Math.abs(camera.near-near)>near*.001||Math.abs(camera.far-far)>.1){
  camera.near=near;camera.far=far;camera.updateProjectionMatrix();
 }
}
function animate(now){
 requestAnimationFrame(animate);
 if(!canvasVisible)return;
 if(animation){const t=Math.max(0,Math.min((now-animation.start)/500,1)),e=t*t*(3-2*t);camera.position.lerpVectors(animation.fromPosition,animation.toPosition,e);controls.target.lerpVectors(animation.fromTarget,animation.toTarget,e);if(t===1)animation=null;}
 controls.update();updateCameraDepth();renderer.render(scene,camera);
 if(ready&&now-lastLabels>90){updateLabels();lastLabels=now;}
}
requestAnimationFrame(animate);
init().catch(error=>{$('loading-text').textContent=`Unable to load the 3D board: ${error.message}`;console.error(error);});
