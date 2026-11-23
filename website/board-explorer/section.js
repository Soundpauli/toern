// Keep the site's fixed shop buttons clear of the board's bottom controls.
const section=document.getElementById('board-explorer');
if(section){
 new IntersectionObserver(([entry])=>{
  document.body.classList.toggle('board-explorer-visible',entry.isIntersecting);
 },{threshold:.15}).observe(section);
}
