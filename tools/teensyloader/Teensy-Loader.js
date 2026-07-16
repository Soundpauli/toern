/**
 * Teensy-Loader.js
 *
 * A small JavaScript library for working with multiple Teensy boards via WebHID and Serial.
 * 
 * Classes:
 *  - FirmwareFile: Manages local firmware data (hex or bin).
 *  - TeensyFlasher: Attempts to flash firmware blocks to a Teensy device over WebHID.
 *  - SerialPortManager: Opens/closes a serial port and handles incoming data.
 * 
 * Updates:
 *  - Supports .hex or .bin on 3.x and 4.x.
 *  - Supports .ehex only on Teensy 4.x (mimics official loader with two-part parse).
 *  - Splits EHEX into main program (flash) + loader utility (RAM).
 *  - Sends both sets of data to the device, using addresses in flash or RAM.
 */

////////////////////////////////////////////////////////////////////////////////
// Utility Functions 
////////////////////////////////////////////////////////////////////////////////

/**
 * Sleep for a specified number of milliseconds.
 * @param {number} ms - milliseconds to sleep
 * @returns {Promise<void>}
 */
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

/**
 * Sends a HID report to a Teensy device, with retries.
 * @private
 * @param {HIDDevice} device - The HID device
 * @param {Uint8Array} data - Data buffer to send
 * @param {number} maxRetries - Maximum number of retry attempts
 * @returns {Promise<boolean>} - True if sent successfully, false otherwise
 */
async function sendReportWithRetries(device, data, maxRetries = 5) {
    for (let attempt = 0; attempt < maxRetries; attempt++) {
        try {
            await device.sendReport(0, data);
            return true;
        } catch (err) {
            console.warn(`sendReport attempt ${attempt + 1} failed:`, err);
            await sleep(100);
        }
    }
    return false;
}

/**
 * Returns a recommended flash offset for each Teensy productId.
 *
 * - Teensy 4.x boards => 0x60000000
 * - Teensy 3.x boards => 0x00000000
 * @private
 */
function getAddressOffset(productId) {
    switch (productId) {
        case 0x0478: // Teensy 4.0
        case 0x0479: // Teensy 4.1
            return 0x60000000;
        default:
            return 0x00000000;
    }
}

/**
 * Check if productId corresponds to a Teensy 3.x
 * @private
 */
function isTeensy3x(productId) {
    // Known 3.x PIDs
    //  3.0 => 0x0483
    //  3.1/3.2 => 0x0484
    //  3.5 => 0x0474
    //  3.6 => 0x0477
    const threeXIds = [0x0483, 0x0484, 0x0474, 0x0477];
    return threeXIds.includes(productId);
}

////////////////////////////////////////////////////////////////////////////////
// Intel HEX Parsing
////////////////////////////////////////////////////////////////////////////////

/**
 * parseSingleHexSession
 * ---------------------
 * Reads a single Intel HEX session (ending at the first EOF record 0x01).
 * Returns an array of objects: 
 *   [ { address: number, data: Uint8Array(1024) }, ... ]
 * where `address` is the actual flash/RAM address for that 1KB block,
 * and `data` is the block’s contents (padded with 0xFF).
 *
 * @param {string} hexText - The raw ASCII text of Intel HEX lines
 * @param {number} blockSize - typically 1024 for Teensy
 * @param {number} offset - e.g. 0x60000000 for T4.x main flash, or 0x00000000 for RAM
 * @returns {Array<{ address: number, data: Uint8Array }>}
 */
function parseSingleHexSession(hexText, blockSize, offset) {
    const lines = hexText.split(/\r?\n/);

    // We store partial data in a Map<blockNumber, Uint8Array>.
    const blockMap = new Map();
    let baseAddress = 0;

    function ensureBlock(blockNum) {
        if (!blockMap.has(blockNum)) {
            const block = new Uint8Array(blockSize);
            block.fill(0xff);
            blockMap.set(blockNum, block);
        }
    }

    lineLoop: for (let lineNum = 0; lineNum < lines.length; lineNum++) {
        let line = lines[lineNum].trim();
        if (!line) continue;                 // skip blank
        if (!line.startsWith(':')) {
            throw new Error(`Invalid HEX line missing ':' (line ${lineNum + 1})`);
        }

        line = line.slice(1);  // remove ':'
        const length = parseInt(line.slice(0, 2), 16);
        const address = parseInt(line.slice(2, 6), 16);
        const recordType = parseInt(line.slice(6, 8), 16);
        let cursor = 8;
        let calcSum = length + ((address >> 8) & 0xff) + (address & 0xff) + recordType;

        // Sanity check line length
        if (line.length !== 10 + length * 2) {
            throw new Error(`Line length mismatch at line ${lineNum + 1}`);
        }

        switch (recordType) {
            case 0x00: {
                // Data record
                const dataBytes = new Uint8Array(length);
                for (let i = 0; i < length; i++) {
                    const byteVal = parseInt(line.slice(cursor, cursor + 2), 16);
                    cursor += 2;
                    dataBytes[i] = byteVal;
                    calcSum += byteVal;
                }
                const checkSum = parseInt(line.slice(cursor, cursor + 2), 16);
                calcSum &= 0xff;
                if (((calcSum + checkSum) & 0xff) !== 0) {
                    throw new Error(`Checksum error at data record, line ${lineNum + 1}`);
                }

                // Now place the data into 1KB blocks
                let addr32 = baseAddress + address;
                addr32 -= offset;     // apply offset for target

                if (addr32 < 0) {
                    // data below region we care about
                    continue;
                }

                let dataIndex = 0;
                while (dataIndex < dataBytes.length) {
                    const blockNum = Math.floor(addr32 / blockSize);
                    const inBlock = addr32 % blockSize;

                    ensureBlock(blockNum);
                    const block = blockMap.get(blockNum);

                    const space = blockSize - inBlock;
                    const toCopy = Math.min(space, dataBytes.length - dataIndex);
                    block.set(dataBytes.subarray(dataIndex, dataIndex + toCopy), inBlock);

                    dataIndex += toCopy;
                    addr32 += toCopy;
                }
                break;
            }

            case 0x01: {
                // EOF record
                // parse leftover data for checks
                for (let i = 0; i < length; i++) {
                    const byteVal = parseInt(line.slice(cursor, cursor + 2), 16);
                    cursor += 2;
                    calcSum += byteVal;
                }
                const checkSum = parseInt(line.slice(cursor, cursor + 2), 16);
                calcSum &= 0xff;
                if (((calcSum + checkSum) & 0xff) !== 0) {
                    throw new Error(`Checksum error at EOF line ${lineNum + 1}`);
                }
                // Return the finalized blocks
                return finalizeBlocks(blockMap, blockSize, offset);
            }

            case 0x02: {
                // Extended Segment Address (baseAddress = highAddr << 4)
                const highAddr = parseInt(line.slice(cursor, cursor + 4), 16);
                calcSum += ((highAddr >> 8) & 0xff) + (highAddr & 0xff);
                cursor += 4;
                const checkSum = parseInt(line.slice(cursor, cursor + 2), 16);
                calcSum &= 0xff;
                if (((calcSum + checkSum) & 0xff) !== 0) {
                    throw new Error(`Checksum error, line ${lineNum + 1}`);
                }
                baseAddress = highAddr << 4;
                break;
            }

            case 0x04: {
                // Extended Linear Address (baseAddress = upper16 << 16)
                const upper16 = parseInt(line.slice(cursor, cursor + 4), 16);
                calcSum += ((upper16 >> 8) & 0xff) + (upper16 & 0xff);
                cursor += 4;
                const checkSum = parseInt(line.slice(cursor, cursor + 2), 16);
                calcSum &= 0xff;
                if (((calcSum + checkSum) & 0xff) !== 0) {
                    throw new Error(`Checksum error, line ${lineNum + 1}`);
                }
                baseAddress = upper16 << 16;
                break;
            }

            default: {
                // Some other record type, do checksum
                for (let i = 0; i < length; i++) {
                    const byteVal = parseInt(line.slice(cursor, cursor + 2), 16);
                    cursor += 2;
                    calcSum += byteVal;
                }
                const checkSum = parseInt(line.slice(cursor, cursor + 2), 16);
                calcSum &= 0xff;
                if (((calcSum + checkSum) & 0xff) !== 0) {
                    throw new Error(`Checksum error on line ${lineNum + 1}`);
                }
                break;
            }
        }
    }

    // If we never encountered EOF, just finalize at end
    return finalizeBlocks(blockMap, blockSize, offset);
}

/**
 * finalizeBlocks
 * --------------
 * Takes a Map<blockNum, Uint8Array> of data, and returns 
 * an array of { address, data } objects, sorted by address.
 * The `address` here is the real memory address: blockNum*blockSize + offset.
 *
 * @param {Map<number, Uint8Array>} blockMap
 * @param {number} blockSize
 * @param {number} offset
 * @returns {Array<{ address: number, data: Uint8Array }>}
 */
function finalizeBlocks(blockMap, blockSize, offset) {
    const result = [];

    for (const [blockNum, blockData] of blockMap.entries()) {
        const realAddress = offset + (blockNum * blockSize);
        result.push({
            address: realAddress,
            data: blockData
        });
    }

    // Sort ascending by address
    result.sort((a, b) => a.address - b.address);
    return result;
}


/**
 * parseEhexFull:
 *  The EHEX file is 2 complete Intel HEX segments concatenated:
 *    1) main flash portion (ends with :00000001FF)
 *    2) loader utility portion (also ends with another :00000001FF)
 *  This function splits the file at the *first* EOF, parse each half separately,
 *  and returns { mainBlocks, loaderBlocks }.
 * 
 *  The main portion uses offset=0x60000000 (typical for Teensy 4.x).
 *  The loader portion is placed in RAM addresses, so offset=0x00000000 is used
 *  (the lines in the second portion presumably start with addresses ~0x2020_0000).
 * 
 * @private
 */
async function parseEhexFull(ehexData, blockSize) {
    const text = new TextDecoder().decode(ehexData);

    // Find the first EOF line: ":00000001FF"
    // Do a manual search for that exact line (with optional whitespace).
    const eofRegex = /^:00000001FF\s*$/m;
    const match = eofRegex.exec(text);

    if (!match) {
        // If there's no first EOF, treat entire text as a single session
        // but it's not a valid EHEX per PJRC. Parse as one segment.
        const mainBlocks = parseSingleHexSession(text, blockSize, 0x60000000);
        return { mainBlocks, loaderBlocks: [] };
    }

    // text up to (and including) that line => main portion
    const firstEOFEnd = match.index + match[0].length;
    const mainText = text.slice(0, firstEOFEnd);
    const afterText = text.slice(firstEOFEnd);

    // parse main
    const mainBlocks = parseSingleHexSession(mainText, blockSize, 0x60000000);

    // parse loader => offset=0, because those lines typically have addresses in RAM
    let loaderBlocks = [];
    if (afterText.trim()) {
        loaderBlocks = parseSingleHexSession(afterText, blockSize, 0x00000000);
    }

    return { mainBlocks, loaderBlocks };
}

////////////////////////////////////////////////////////////////////////////////
// FirmwareFile
////////////////////////////////////////////////////////////////////////////////

export class FirmwareFile {
    /**
     * @param {Uint8Array} fileData - The file data
     * @param {string} filename     - The file name
     * @param {number} productId    - The Teensy product ID (0x0478, 0x0479, etc.)
     */
    constructor(fileData, filename, productId) {
        this.fileData = fileData;
        this.filename = filename.toLowerCase();
        this.productId = productId || 0;
    }

    /**
     * Builds 1KB firmware blocks. 
     * - For Teensy 3.x: we only allow .hex or .bin. .ehex => error.
     * - For Teensy 4.x:
     *    - .hex => single parse
     *    - .ehex => parse in two segments
     *    - .bin => raw blocks
     *
     * Returns: { mainBlocks: Uint8Array[], loaderBlocks: Uint8Array[] }
     */
    async buildBlocks() {
        const PAGE_SIZE = 1024;

        const is3x = isTeensy3x(this.productId);
        const is4x = !is3x; // or specifically check for 0x0478, 0x0479

        // If .ehex on 3.x => not supported
        if (is3x && this.filename.endsWith('.ehex')) {
            throw new Error(`EHEX not supported on Teensy 3.x (productId=0x${this.productId.toString(16)})`);
        }

        // .hex => single parse
        if (this.filename.endsWith('.hex')) {
            const offset = getAddressOffset(this.productId);
            const text = new TextDecoder().decode(this.fileData);
            const mainBlocks = parseSingleHexSession(text, PAGE_SIZE, offset);
            return { mainBlocks, loaderBlocks: [] };
        }

        // .ehex => (Teensy 4.x only)
        if (this.filename.endsWith('.ehex') && is4x) {
            const { mainBlocks, loaderBlocks } = await parseEhexFull(this.fileData, PAGE_SIZE);
            return { mainBlocks, loaderBlocks };
        }

        // .bin => raw binary, split into 1KB pages with addresses
        const offset = getAddressOffset(this.productId);
        const mainBlocks = [];
        for (let i = 0; i < this.fileData.length; i += PAGE_SIZE) {
            const chunk = this.fileData.slice(i, i + PAGE_SIZE);
            let data;
            if (chunk.length < PAGE_SIZE) {
                data = new Uint8Array(PAGE_SIZE);
                data.fill(0xff);
                data.set(chunk, 0);
            } else {
                data = chunk;
            }
            mainBlocks.push({ address: offset + i, data });
        }
        return { mainBlocks, loaderBlocks: [] };
    }
}

////////////////////////////////////////////////////////////////////////////////
// TeensyFlasher
////////////////////////////////////////////////////////////////////////////////

export class TeensyFlasher {
    /**
     * Flash the firmware onto the Teensy.
     * 
     * blocksObj.mainBlocks / blocksObj.loaderBlocks 
     *   => arrays of { address, data } from parseSingleHexSession
     * 
     * @param {{ mainBlocks: Array<{address:number, data:Uint8Array}>, 
     *           loaderBlocks: Array<{address:number, data:Uint8Array}>}} blocksObj
     * @param {HIDDevice} device
     * @param {(progress: number) => void} [progressCb]
     */
    async flashFirmware(blocksObj, device, progressCb) {
        if (!progressCb) progressCb = () => { };

        const { mainBlocks, loaderBlocks } = blocksObj;

        // Attempt to open device
        await device.open();
        try {
            // Count total blocks that need sending
            const totalNeeded = this.countNeeded(mainBlocks) + this.countNeeded(loaderBlocks);
            let doneCount = 0;

            // 1) Send main (flash) blocks
            await this.sendBlocks(mainBlocks, device, count => {
                doneCount += count;
                progressCb(doneCount / totalNeeded);
            });

            // 2) Send loader (RAM) blocks
            if (loaderBlocks.length > 0) {
                await this.sendBlocks(loaderBlocks, device, count => {
                    doneCount += count;
                    progressCb(doneCount / totalNeeded);
                });
            }

            // 3) Final “magic” = 0xFF, 0xFF, 0xFF
            const PAGE_SIZE = 1024;
            const REPORT_SIZE = PAGE_SIZE + 64;
            const finalReport = new Uint8Array(REPORT_SIZE);
            finalReport[0] = 0xff;
            finalReport[1] = 0xff;
            finalReport[2] = 0xff;
            await sendReportWithRetries(device, finalReport, 5);

            // small delay
            await sleep(100);
        } finally {
            await device.close().catch(() => { });
        }
    }

    /**
     * sendBlocks
     * ----------
     * Sends an array of { address, data } blocks via HID, skipping blocks that are fully 0xFF
     * (except possibly the first).
     *
     * @private
     * @param {Array<{ address:number, data:Uint8Array }>} blocks
     * @param {HIDDevice} device
     * @param {(count:number)=>void} incCb - increment progress
     */
    async sendBlocks(blocks, device, incCb) {
        const PAGE_SIZE = 1024;
        const REPORT_SIZE = PAGE_SIZE + 64;
        const report = new Uint8Array(REPORT_SIZE);

        // Filter to needed blocks e.g. skip if block.data is all 0xFF
        const needed = blocks.filter((b, i) => {
            if (i === 0) return true; // always include first block
            return !b.data.every(x => x === 0xff);
        });

        let sentCount = 0;
        for (const block of needed) {
            report.fill(0);

            // Put the real address in the first 3 bytes (24-bit address)
            const addr = block.address;
            // If addresses exceed 24 bits, you might need a 4th byte 
            // depending on how the Teensy bootloader is implemented.
            report[0] = addr & 0xff;
            report[1] = (addr >> 8) & 0xff;
            report[2] = (addr >> 16) & 0xff;

            // copy the data at offset 64
            report.set(block.data, 64);

            const success = await sendReportWithRetries(device, report, 5);
            if (!success) {
                throw new Error(`Block upload failed at address=0x${addr.toString(16)}`);
            }

            sentCount++;
            if (incCb) incCb(1);

            // Delay: 1.5s after first block (erase), 5ms for subsequent
            await sleep(sentCount === 1 ? 1500 : 5);
        }
        return sentCount;
    }

    /**
     * countNeeded
     * -----------
     * Helper to count how many blocks aren't entirely 0xFF 
     * (except you might choose to keep block #0).
     *
     * @private
     */
    countNeeded(blocks) {
        return blocks.filter((b, i) => {
            if (i === 0) return true;
            return !b.data.every(x => x === 0xff);
        }).length;
    }
}


////////////////////////////////////////////////////////////////////////////////
// SerialPortManager
////////////////////////////////////////////////////////////////////////////////

export class SerialPortManager {
    constructor() {
        this.serialPort = null;
        this.serialReader = null;
        this.incompleteLine = '';
        this.onData = null;
    }

    async openSerialPort(serialOptions = { baudRate: 115200 }) {
        if (this.serialPort) {
            throw new Error('Serial port is already open.');
        }
        this.serialPort = await navigator.serial.requestPort();
        await this.serialPort.open(serialOptions);

        const textDecoder = new TextDecoderStream();
        this.serialPort.readable.pipeTo(textDecoder.writable);
        this.serialReader = textDecoder.readable.getReader();

        (async () => {
            try {
                while (true) {
                    const { value, done } = await this.serialReader.read();
                    if (done) break;
                    this.processSerialData(value);
                }
            } catch (err) {
                console.error('Serial reading error:', err);
            }
        })();
    }

    processSerialData(dataChunk) {
        this.incompleteLine += dataChunk;
        const lines = this.incompleteLine.split('\n');

        for (let i = 0; i < lines.length - 1; i++) {
            const line = lines[i].trim();
            if (this.onData) {
                this.onData(line);
            }
        }
        this.incompleteLine = lines[lines.length - 1];
    }

    async closeSerialPort() {
        if (!this.serialPort) {
            throw new Error('No serial port is open.');
        }
        if (this.serialReader) {
            await this.serialReader.cancel();
            this.serialReader = null;
        }
        await this.serialPort.close();
        this.serialPort = null;
        this.incompleteLine = '';
    }
}

/**
 * Device filters to include multiple Teensy boards (3.x and 4.x).
 */
export const TEENSY_DEVICE_FILTERS = [
    { vendorId: 0x16c0, productId: 0x0478 }, // Teensy 4.0
    { vendorId: 0x16c0, productId: 0x0479 }, // Teensy 4.1
    { vendorId: 0x16c0, productId: 0x0477 }, // Teensy 3.6
    { vendorId: 0x16c0, productId: 0x0474 }, // Teensy 3.5
    { vendorId: 0x16c0, productId: 0x0484 }, // Teensy 3.1 / 3.2
    { vendorId: 0x16c0, productId: 0x0483 }, // Teensy 3.0
];
