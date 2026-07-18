document.documentElement.classList.add("fonts-loaded");
if (document.fonts?.ready) {
  document.fonts.ready.then(() => {
    document.documentElement.classList.add("fonts-loaded");
  });
}
