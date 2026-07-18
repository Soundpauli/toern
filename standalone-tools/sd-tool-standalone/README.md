# TŒRN SD tool (standalone)

Serial file explorer for the TŒRN SD card. The unit does **not** mount the card as USB mass storage; transfers use the USB Serial file server while **Menu → ETC → SD** is open.

## Hosted site (Web Serial)

https://sdtool.tyng.app serves **static files**. The browser talks to the Teensy over **Web Serial**.

### Usage

1. On the device: **Menu → ETC → SD** (screen shows **WAIT**, then **OK** when a host connects)
2. On a computer: open the site in **Chrome or Edge** (desktop)
3. Click **Connect** and pick the Teensy serial port
4. Browse, upload, download, rename, or delete files — stay on **ETC → SD** for the whole session

**Notes:**

- Desktop Chrome/Edge only. Android / Pixel Chrome usually cannot see the Teensy USB serial port.
- Opening the serial port can reboot the Teensy — if the screen leaves the SD page, open **ETC → SD** again after Connect.
- WAV uploads are converted in the browser to **44.1 kHz mono 16-bit**.
- Drag-and-drop accepts **multiple files**, **folders** (nested paths kept), and **`.zip`** archives (extracted in the browser into the current folder). Use **Folder** in the toolbar to pick a directory.

### Deploy to `/var/www/sdtool.tyng.app`

```bash
rsync -avz --delete \
  /Users/jank/git/toern/tools/sd-tool-standalone/index.html \
  /Users/jank/git/toern/tools/sd-tool-standalone/styles.css \
  /Users/jank/git/toern/tools/sd-tool-standalone/tool-shell.css \
  /Users/jank/git/toern/tools/sd-tool-standalone/favicon.svg \
  root@tyng.app:/var/www/sdtool.tyng.app/
```

Apache should use **DocumentRoot** (no Python reverse proxy):

```apache
<VirtualHost *:80>
    ServerName sdtool.tyng.app
    DocumentRoot /var/www/sdtool.tyng.app
    <Directory /var/www/sdtool.tyng.app>
        Options -Indexes +FollowSymLinks
        AllowOverride None
        Require all granted
        DirectoryIndex index.html
    </Directory>
</VirtualHost>
```

Then `certbot --apache -d sdtool.tyng.app` for HTTPS.

Disable the old Python service if it was installed:

```bash
systemctl disable --now toern-sdtool
```

## Local CLI / local web (Python + pyserial)

Still useful for scripting or when Web Serial is unavailable:

```bash
pip install pyserial
python3 toern_sd.py ports
python3 toern_sd.py -p /dev/cu.usbmodemXXXX list /
python3 toern_sd.py web   # http://127.0.0.1:8787 (also uses explorer.html)
```

Requires USB Type with Serial (e.g. **Serial + MIDI**). Protocol commands include ACK-framed `GET` / `PUT`.
