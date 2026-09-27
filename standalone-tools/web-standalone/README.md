# TŒRN simulator (standalone)

Early browser emulator of the TŒRN groovebox — concept / feel sketch, not a finished product.

## Hosted site

https://sim.tyng.app serves the Vite **build output** (`dist/`) as static files.

### Local develop

```bash
cd standalone-tools/web-standalone
npm install
npm run dev    # http://127.0.0.1:5173
```

Sample / pattern files come from `SD-CARD-CONTENT/` (dev server mounts them at `/sd/`).

### Build

```bash
npm run build   # writes dist/ (+ copies SD-CARD-CONTENT → dist/sd/)
```

### Deploy to `/var/www/sim.tyng.app`

```bash
npm run build
rsync -avz --delete \
  dist/ \
  root@tyng.app:/var/www/sim.tyng.app/
```

Apache **DocumentRoot** (same pattern as sdtool):

```apache
<VirtualHost *:80>
    ServerName sim.tyng.app
    DocumentRoot /var/www/sim.tyng.app
    <Directory /var/www/sim.tyng.app>
        Options -Indexes +FollowSymLinks
        AllowOverride None
        Require all granted
        DirectoryIndex index.html
    </Directory>
</VirtualHost>
```

Then `certbot --apache -d sim.tyng.app` for HTTPS.
