import * as THREE from 'three';

export function addEncoderAssemblies({scene, moduleModel, rotaryModel, byRef, objects, boxes, pickMeshes, origin}){
 const assemblies=new THREE.Group();assemblies.name='Encoder assemblies';scene.add(assemblies);
 const added=[];
 const colors=[0xff3b30,0x38b6ff,0xffbf39,0x53df86];
 const boardColor=new THREE.Color(0x0b512f),plastic=new THREE.Color(0x363a3e);
 const boardMaterials=new Map();
 moduleModel.traverse(mesh=>{if(!mesh.isMesh)return;
  const source=mesh.material;
  if(!boardMaterials.has(source)){
   const m=source.clone();m.metalness=.05;m.roughness=.75;
   if(Math.max(m.color.r,m.color.g,m.color.b)<.02){
    m.color.copy(plastic);
    // The upstream STEP combines the black board and black component solids
    // in one mesh. Its board occupies z = -1.6..0 mm; parts sit below it.
    m.onBeforeCompile=shader=>{
     shader.vertexShader='varying float vEncoderZ;\n'+shader.vertexShader;
     shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\nvEncoderZ = position.z;');
     shader.uniforms.uEncoderBoardColor={value:boardColor};
     shader.fragmentShader='varying float vEncoderZ;\nuniform vec3 uEncoderBoardColor;\n'+shader.fragmentShader;
     shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',`#include <color_fragment>
      diffuseColor.rgb = mix(diffuseColor.rgb, uEncoderBoardColor, step(-1.6001, vEncoderZ));`);
    };
    m.customProgramCacheKey=()=> 'encoder-board-finish-v2-color';
   }else if(Math.min(m.color.r,m.color.g,m.color.b)>.4){m.metalness=.8;m.roughness=.35;}
   boardMaterials.set(source,m);
  }
  mesh.material=boardMaterials.get(source);
 });
 const headerPlastic=new THREE.MeshStandardMaterial({color:0x353a3e,roughness:.85,metalness:0});
 const pinMetal=new THREE.MeshStandardMaterial({color:0xd2aa54,roughness:.3,metalness:.8});
 const pairs=[['J2','J18'],['J19','J20'],['J21','J22'],['J23','J24']];
 function register(ref,node){
  objects.set(ref,node);node.updateWorldMatrix(true,true);
  boxes.set(ref,new THREE.Box3().setFromObject(node));
  node.traverse(mesh=>{if(mesh.isMesh){mesh.userData.ref=ref;pickMeshes.push(mesh);}});
 }
 for(const [i,pair] of pairs.entries()){
  const ref=`ENC${i+1}`,rotaryRef=`${ref}.ROT1`,left=byRef.get(pair[0]),right=byRef.get(pair[1]);
  const x=(left.x+right.x)/2,z=left.z+5.08;
  const socketTop=Math.max(boxes.get(left.ref).max.y,boxes.get(right.ref).max.y)+origin.y;
  const spacerHeight=2.54,thickness=1.6,top=socketTop+spacerHeight+thickness;
  const assembly=new THREE.Group();assembly.name=ref;assembly.position.set(x-origin.x,top-origin.y,z-origin.z);assemblies.add(assembly);
  const board=new THREE.Group();assembly.add(board);
  const module=moduleModel.clone(true);module.rotation.x=-Math.PI/2;board.add(module);
  const pads=[];
  for(const [side,header] of [['L',left],['R',right]]){
   const offset=header.x-x;
   const spacer=new THREE.Mesh(new THREE.BoxGeometry(2.54,spacerHeight,12.7),headerPlastic);
   spacer.position.set(offset,-thickness-spacerHeight/2,0);board.add(spacer);
   for(let pin=0;pin<5;pin++){
    const pz=(pin-2)*2.54;
    const contact=new THREE.Mesh(new THREE.BoxGeometry(.64,8.5,.64),pinMetal);
    contact.position.set(offset,-3.55,pz);board.add(contact);
    const mainPad=header.pads.find(p=>p.number===String(pin+1));
    pads.push({number:`${side}${pin+1}`,net:mainPad.net,x:header.x,z:z+pz,size:[1.43,1.43],height:top,peers:[...mainPad.peers]});
   }
  }
  const rotary=rotaryModel.clone(true);assembly.add(rotary);
  const rotaryMaterials=new Map();
  rotary.traverse(mesh=>{if(!mesh.isMesh)return;const source=mesh.material;
   if(!rotaryMaterials.has(source)){
    const m=source.clone();const c=m.color;
    if(c.g>c.r*1.4&&c.g>c.b*1.4){
     m.color.set(colors[i]);m.emissive.set(colors[i]);m.emissiveIntensity=.18;m.metalness=0;m.roughness=.3;
    }else if(Math.max(c.r,c.g,c.b)<.03){m.color.copy(plastic);m.metalness=0;m.roughness=.8;}
    else{m.metalness=.82;m.roughness=.32;}
    rotaryMaterials.set(source,m);
   }
   mesh.material=rotaryMaterials.get(source);
  });
  register(ref,board);register(rotaryRef,rotary);
  const base={height:top,category:'Encoders',side:'Assembly',angle:0,x,z,lcsc:'',datasheet:'',footprint:'DuPPa I2C Encoder V2.1 · upstream STEP'};
  added.push({...base,ref,value:'I2C Encoder V2.1',title:`I²C encoder board ${i+1}`,pads,
   description:`25 × 25 mm DuPPa / Simone Caron encoder module on ${pair.join(' + ')}. The two five-pin header rows are 21 mm apart. Both rows carry GND, +5 V, SDA, SCL and the shared GPIO27 / interrupt connection. The board model includes its underside electronics. Select ${rotaryRef} for the RGB rotary encoder.`,
   caution:`Assembly height is estimated: 2.54 mm male-header spacer above the actual carrier socket, plus a 1.6 mm board. This is the upstream v2.1 STEP; the local TC2030 ICSP modification is not represented.`});
  const rgbPins=[['A','ENC_A',-2.500122,7.500112],['C','GND',0,7.500112],['B','ENC_B',2.500122,7.500112],['LR','PWM_LR_GP3',-3.999992,-6.999986],['LG','PWM_LG_SW',-1.999996,-6.999986],['SW','SW_RGB',0,-6.999986],['LB','PWM_LB_SWC',1.999996,-6.999986],['COM','VCC',3.999992,-6.999986]];
  added.push({...base,ref:rotaryRef,value:'RGB rotary encoder',title:`RGB rotary encoder ${i+1}`,footprint:'Encoder:ROT1_DuPPa',
   description:`Upstream RGB encoder STEP mounted at the center of ${ref}. It combines a mechanical quadrature encoder, push switch and common-anode RGB illumination. Local pin names come from ROT1 in the encoder KiCad PCB.`,
   caution:'Shaft illumination colors are illustrative, not live firmware status. Placement uses the source model; the installed header stack height is estimated.',
   pads:rgbPins.map(([number,net,px,pz])=>({number,net,x:x+px,z:z+pz,size:[1.5,1.5],height:top,peers:[{ref,pin:'module'}]}))});
 }
 for(const part of added)byRef.set(part.ref,part);
 // The fitted modules are additional peers on the carrier's shared bus.
 // Rotary-switch / LED pin names remain local to each daughterboard.
 const carrierParts=[...byRef.values()].filter(p=>!p.ref.endsWith('.ROT1'));
 const netMembers=new Map();
 for(const part of carrierParts)for(const pad of part.pads){
  if(!pad.net)continue;
  if(!netMembers.has(pad.net))netMembers.set(pad.net,[]);
  netMembers.get(pad.net).push({ref:part.ref,pin:pad.number});
 }
 for(const part of carrierParts)for(const pad of part.pads)if(pad.net)pad.peers=netMembers.get(pad.net);
 return {group:assemblies,parts:added,setColor:color=>boardColor.set(color)};
}
