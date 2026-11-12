import { resolve } from 'node:path';
import { defineConfig } from 'vite';

export default defineConfig({
  root: '.',
  publicDir: false,
  base: './',
  assetsInclude: ['**/*.hex'],
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
        ramTest: resolve(__dirname, 'ram-test.html'),
      },
    },
  },
});
