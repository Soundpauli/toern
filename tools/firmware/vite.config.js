import { defineConfig } from 'vite';

export default defineConfig({
  root: '.',
  publicDir: false,
  base: './',
  assetsInclude: ['**/*.hex'],
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
  },
});
