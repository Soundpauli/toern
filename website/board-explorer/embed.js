// Opening component details is always an explicit visitor action.
const toggle=document.getElementById('inspector-toggle');
const inspector=document.getElementById('inspector');
if(toggle&&inspector){
 function setOpen(open){
  document.body.classList.toggle('inspector-open',open);
  toggle.setAttribute('aria-expanded',String(open));
  toggle.setAttribute('aria-label',open?'Close component details':'Open component details');
  inspector.setAttribute('aria-hidden',String(!open));
  inspector.inert=!open;
  if(!open&&inspector.contains(document.activeElement))toggle.focus();
 }
 toggle.addEventListener('click',()=>setOpen(toggle.getAttribute('aria-expanded')!=='true'));
 document.addEventListener('keydown',event=>{
  if(event.key==='Escape'&&toggle.getAttribute('aria-expanded')==='true'){
   setOpen(false);toggle.focus();
  }
 });
 setOpen(false);
}
