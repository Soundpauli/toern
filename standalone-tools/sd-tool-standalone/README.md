# TŒRN SD tool (standalone)

Serial file explorer for the TŒRN SD card. The unit does **not** mount the card as USB mass storage. Transfers use the USB Serial file server from any screen.

**Menu → ETC → SD** shows **SCON**: **WAIT** in yellow until a host is talking, then **OK** in green. A USB cable with no Connect dialog stays on WAIT and does not stall the instrument.

## Hosted site (Web Serial)

https://sdtool.toern.live serves **static files**. The browser talks to the Teensy over **Web Serial**.

### Usage

1. On a computer: open the site in **Chrome or Edge** (desktop)
2. Click **Connect** and pick the Teensy serial port
3. Browse, upload, download, rename, or delete files

**Notes:**

- Desktop Chrome/Edge only. Android / Pixel Chrome usually cannot see the Teensy USB serial port.
- Opening the serial port can reboot the Teensy — connect again after it comes back.
- WAV uploads are converted in the browser to **44.1 kHz mono 16-bit**.
- Drag-and-drop accepts **multiple files**, **whole folders with subfolders and empty folders**, and **`.zip`** archives (extracted in the browser into the current folder). Use **Upload folder(s)** in the toolbar to pick a directory; browser folder pickers expose files and their paths, while drag-and-drop also preserves empty directories.
- Folder/file names with spaces require firmware advertising `PUT_SPACES` in its serial greeting. Reflash the firmware built from this checkout, then reconnect. Older firmware misparsed spaces as file-size/CRC fields; the browser now rejects those uploads before writing anything.
- Each upload stays in the SD folder selected when it began. Existing folders are merged; hidden files/folders are skipped. Paths are checked against the device's 127-byte UTF-8 limit before transfer.

### Deploy to `/var/www/sdtool.tyng.app`

```bash
rsync -avz --delete \
  /Users/jank/git/toern/standalone-tools/sd-tool-standalone/index.html \
  /Users/jank/git/toern/standalone-tools/sd-tool-standalone/styles.css \
  /Users/jank/git/toern/standalone-tools/sd-tool-standalone/tool-shell.css \
  /Users/jank/git/toern/standalone-tools/sd-tool-standalone/tool-theme.css \
  /Users/jank/git/toern/standalone-tools/sd-tool-standalone/favicon.svg \
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

## Regression checks

```bash
node tests/test_sd_tool_upload.mjs
python3 tests/test_sd_upload_protocol.py
pio run -e teensy41
```

Run from the repository root. The browser regression checks the actual inline JavaScript in both entry points. The firmware check compiles the actual PUT handler against an in-memory SD card, including spaced paths, ACK blocks and CRC rejection.

### Diagnosing interrupted uploads

There is no 3 MB upload cutoff; firmware accepts files up to 512 MiB. WAVs may become smaller during conversion. The debug log records source size, converted transfer size/CRC, READY, progress every 256 KiB and the last acknowledged byte on failure. A timed-out PUT closes the serial session rather than sending keepalive text into an unfinished binary payload. Browser writes have their own deadline. Keepalive continues during local conversion, but is suppressed during serial transactions.

After a device stops replying to PING, power-cycle it and connect again. A software transfer test cannot establish whether the physical card, firmware or USB link caused a hardware stall.

### Large folder and ZIP batches

ZIP uploads read a bounded central-directory index, then decompress, verify and convert one entry at a time during upload. The browser no longer retains the whole archive as a JavaScript ArrayBuffer plus every extracted file. Directory drops already read file content one file at a time. Archives are checked for entry size/CRC corruption before writing file bytes to the device.

The supplied `Archiv.zip` was checked in desktop Chrome: all 175 visible files (270,093,240 original bytes) extracted and converted successfully to 63,292,654 bytes. Indexing read 102,900 bytes with a largest single read of 65,557 bytes. This browser check does not establish that the attached SD hardware can complete the transfer.

Firmware diagnostics: `SDINFO` reports CPU speed, temperature and card error status. Explicit `PUTTRACE` uses the usual PUT arguments and adds `TRACE` phase/offset lines for a diagnostic host; ordinary `PUT` framing stays unchanged. `PUT_TRACE` in the greeting advertises this support.

The current external-touch device (`exttouch = true`, USB serial `202504101`) requires a physical restart after flashing before testing. It reproduced missing receive bytes inside an 8 KB host block, followed by the firmware's 60-second `ERR IO` timeout. Uploads now buffer the complete block before SD writes and consume the USB core's single-byte API: the bulk reader re-enables interrupts while requeueing packets and continues processing its receive ring. Hardware validation with this reader uploaded all 175 browser-converted archive files (63,292,654 bytes), downloaded each one and compared every byte successfully; the complete round trip and cleanup took 297 seconds. The test used ordinary PUT framing without tracing or added host pacing. A separate 5,346,474-byte unconverted `roots 808.wav` also passed upload and exact readback (CRC `028A5ADE`) in 24 seconds. All temporary hardware test files were removed.
