"""Prepare explainer data from the current KiCad PCB, without editing it."""
import json, math, hashlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[1] / 'hardware/PCB/toern_revJ/toern_revJ.kicad_pcb'
data = json.loads((HERE / 'source-data.json').read_text())
notes = {
 'J1': ('Headphones', 'Audio', '6.35 mm TRS headphone output from the SGTL5000 codec. Tip is left, ring is right. Sleeve uses the codec headphone virtual ground, HP_VGND.', 'The sleeve is HP_VGND, not the board GND net.'),
 'J3': ('MIDI input', 'Audio', '3.5 mm TRS MIDI input. The signal feeds the TLP2361 optocoupler U4 through the input protection network. The sleeve pad is unconnected.', ''),
 'J4': ('USB device / power', 'I/O', 'USB-C device port: D+ and D− connect to the Teensy device pogo contacts CN2 and CN1. VBUS feeds the charging and power circuit. R9 and R10 are 5.1 kΩ CC pull-downs.', ''),
 'J5': ('Stereo line output', 'Audio', '6.35 mm TRS line output. Left and right codec signals are AC-coupled through C17 and C16. Sleeve is GND.', ''),
 'J6': ('Stereo line input', 'Audio', '6.35 mm TRS line input. Tip and ring feed the codec line-input network through C19 and C18. Sleeve is GND.', ''),
 'J7': ('External microphone', 'Audio', '6.35 mm microphone input. Tip feeds the microphone path through C20; the normalled tip contact carries MICBIAS. Ring is unconnected.', ''),
 'J8': ('microSD storage', 'I/O', 'Hirose microSD socket wired for SPI: CS on Teensy GPIO10, MOSI on GPIO11, MISO on GPIO12, and SCK on GPIO13. Supply is +3.3 V.', ''),
 'J9': ('Switched 5 V + GPIO', 'JST', '3-pin JST PH. Pin 1 goes through R20 (220 Ω) to Teensy GPIO17; pin 2 is GND; pin 3 is the output of the TPS22918 load switch U8. U8 is enabled by GPIO36.', 'Pin 3 is switched power. The PCB does not give this connector a more specific functional name.'),
 'J10': ('Speaker output', 'JST', '2-pin JST PH connected to the PAM8403 amplifier’s right-channel bridge outputs: pin 1 to U13 pin 14 (ROUT+), pin 2 to U13 pin 16 (ROUT−).', 'Both pins are amplifier outputs; neither pin is GND.'),
 'J11': ('GPIO breakout · 3.3 V', 'JST', '5-pin JST PH: GPIO24, GPIO25, GPIO31, +3.3 V, GND. The nearby silkscreen reads “I2S2-3V”; the table preserves the actual PCB net names.', ''),
 'J11.5': ('GPIO breakout · 5 V', 'JST', '5-pin JST PH sharing GPIO24, GPIO25 and GPIO31 with J11. Pin 4 is +5 V instead of +3.3 V. Nearby silkscreen reads “I2S2-5V”.', 'The supply differs from J11. The GPIO nets are shared directly; a 5 V supply label does not establish 5 V GPIO tolerance.'),
 'J12': ('GPIO 22 / 5 / 15', 'JST', '5-pin JST PH breakout: GPIO22, GPIO5, GPIO15, +3.3 V and GND. Alternative peripheral functions are preserved in the pin net names.', ''),
 'J13': ('Touch buttons', 'JST', 'Five touch-button inputs for the standard panel (exttouch=false). Pin 1: Touch 1 / Single (GPIO2). Pin 2: Touch 3 / Record (GPIO4). Pin 3: Touch 2 / Menu (GPIO3). Pin 4: Touch 4 / Audition (GPIO6). Pin 5: Touch 5 / FN (GPIO39). Mapping verified against docs/BUILD.md and platformio/toern.ino.', 'No power or GND pin. OUT2, BCLK2 and LRCLK2 in the PCB net names are alternate GPIO functions; the standard firmware uses these pins for the touch panel.'),
 'J14': ('GPIO 14 / 9 / 16', 'JST', '5-pin JST PH breakout: GPIO14, GPIO9, GPIO16, +3.3 V and GND. Aliases include SPDIF_OUT and SCL1; actual behavior depends on firmware.', ''),
 'J15': ('GPIO 33 / 32 / 41', 'JST', '5-pin JST PH breakout: GPIO33, GPIO32, GPIO41, +3.3 V and GND. Net aliases include MCLK2, OUT1B and A17.', ''),
 'J16': ('Battery', 'JST', '2-pin JST PH battery connector. Pin 1 is GND; pin 2 is VBAT, connected to the BQ24075 charger / power-path IC.', ''),
 'J17': ('MIDI output', 'Audio', '3.5 mm TRS MIDI output. Pin 2 connects through R7 to the MIDI output driver; pin 3 connects through R8 to +3.3 V. Pin 1 is GND.', ''),
 'J25': ('Clock output · 5 V', 'JST', '2-pin JST PH labeled CLK-OUT(5V). Signal travels from GPIO37 through U15 (74AHCT1G125) and R28. Pin 2 is GND.', ''),
 'J26': ('Clock input · 3–5 V', 'JST', '2-pin JST PH labeled CLK-IN(3-5V). Pin 1 feeds R30, the BAT54S clamp D2, and the Schmitt buffer U14, then Teensy GPIO38. Pin 2 is GND.', '3–5 V is the board’s silkscreen label, not a new electrical qualification.'),
 'J27': ('Sidechain output · 5 V', 'JST', '2-pin JST PH labeled SIDEC-5V. Pin 1 is driven by GPIO28 through U16 (74AHCT1G125) and R32. Pin 2 is GND.', ''),
 'J33': ('USB host', 'I/O', 'The revJ USB-C host port. VBUS is 5V-HOST; data is D+HOST and D-HOST. These signals connect to the Teensy host pogo row CN3–CN7. R11 and R33 are separate 56 kΩ CC pull-ups.', 'This is the host port. The design README specifies no additional CC controller or carrier VBUS switch.'),
 'U6': ('Teensy 4.1', 'Core', 'Main controller module. Its carrier footprint exposes the numbered GPIO, audio buses and power rails. CN1/CN2 pick up device USB data; CN3–CN7 pick up the USB host interface.', 'The pad table describes the carrier footprint. The detailed Teensy 3D assembly is a visual model, not a netlist for the module internals.'),
 'U3': ('Audio codec', 'Core', 'SGTL5000 stereo audio codec. Connects the Teensy digital audio interface to line input, line output, headphones and microphone circuitry.', ''),
 'U13': ('Speaker amplifier', 'Core', 'PAM8403 class-D stereo amplifier. The carrier uses its right-channel bridge output at J10. GPIO30 connects to the amplifier shutdown input.', ''),
 'U4': ('MIDI optocoupler', 'Core', 'TLP2361 optocoupler in the MIDI input path, separating the incoming MIDI current-loop signal from the controller-side logic.', ''),
 'U5': ('MIDI logic buffer', 'Core', '74LVC2G14 dual inverting Schmitt trigger used in the MIDI circuitry. Select a pin to inspect its direct net connections.', ''),
 'U14': ('Clock input buffer', 'Core', '74LVC1G17 Schmitt buffer between the protected clock input network and GPIO38.', ''),
 'U15': ('Clock output driver', 'Core', '74AHCT1G125 buffer supplied by +5 V. Drives J25 through R28 from GPIO37.', ''),
 'U16': ('Sidechain driver', 'Core', '74AHCT1G125 buffer supplied by +5 V. Drives J27 through R32 from GPIO28.', ''),
 'U1': ('Analog 3.3 V regulator', 'Power', 'SPX3819 linear regulator: +5 V input to +3.3VA for the codec analog supply.', ''),
 'U2': ('1.8 V regulator', 'Power', 'SPX3819 linear regulator: +5 V input to the +1V8 rail.', ''),
 'U7': ('Digital 3.3 V regulator', 'Power', 'SPX3819 linear regulator: +5 V input to the +3.3V rail.', ''),
 'U8': ('Switched peripheral power', 'Power', 'TPS22918 load switch. +5 V enters pin 1, GPIO36 controls ON at pin 3, and output pin 6 supplies J9 pin 3.', ''),
 'IC1': ('Battery charger / power path', 'Power', 'BQ24075 Li-ion charger and power-path manager. VBUS enters from J4, VBAT connects to J16, and VYS feeds the selectable power path.', ''),
 'S1': ('Power switch', 'Power', 'EG2219 dual-pole slide switch between the selected source net from JP1 and the carrier +5 V rail.', ''),
 'JP1': ('Power source jumper', 'Power', 'Three-pad solder jumper. Center pad feeds S1; outer pads select USB VBUS or charger output VYS.', ''),
 'MK1': ('Onboard microphone', 'Audio', 'Electret microphone capsule connected to the codec microphone and bias circuitry. Inspect the pad nets for its exact carrier connections.', ''),
 'D2': ('Clock input clamp', 'Core', 'BAT54S dual Schottky clamp. Reviewed revJ mapping: pin 1 = GND, pin 2 = +3.3 V, pin 3 = common at the clock-input buffer network.', ''),
 'R9': ('USB device CC1 pull-down', 'Passives', '5.1 kΩ from J4 CC1 to GND, identifying the device-side port.', ''),
 'R10': ('USB device CC2 pull-down', 'Passives', '5.1 kΩ from J4 CC2 to GND, identifying the device-side port.', ''),
 'R11': ('USB host CC1 pull-up', 'Passives', '56 kΩ from J33 CC1 to 5V-HOST, as specified in the revJ README.', ''),
 'R33': ('USB host CC2 pull-up', 'Passives', '56 kΩ from J33 CC2 to 5V-HOST, as specified in the revJ README.', ''),
}
for r in ['J2'] + [f'J{i}' for i in range(18,25)]:
 notes[r] = ('I²C / peripheral header', 'I/O', '5-contact 2.54 mm socket. Shared bus pinout: GND, +5 V, SDA (GPIO18), SCL (GPIO19), and GPIO27. All eight sockets share these same nets.', '')
for i in range(1,8):
 signal = ['USB device D−','USB device D+','Host +5 V','Host D−','Host D+','Host GND','Host GND'][i-1]
 notes[f'CN{i}'] = (signal+' pogo', 'I/O', 'Spring contact to the underside of the Teensy module. '+ ('Device USB contact.' if i < 3 else 'Part of the five-contact USB host row.') + ' YZP0354-20100-01.', '')
for i in range(28,32):
 notes[f'J{i}'] = ('Teensy socket assembly', 'Core', '3M 929870-01-12-RA assembly model for the Teensy socket. This assembly footprint has no electrical pads; electrical connections are represented by U6.', '')

by_net = {}
for c in data['parts']:
 for pad in c['pads']:
  if len(pad['net']) > 1 and pad['number']:
   by_net.setdefault(pad['net'][1], []).append({'ref':c['ref'],'pin':pad['number']})

parts=[]
for c in data['parts']:
 ref=c['ref']
 if ref == 'LOGOOPENHARDWARE': continue
 fallback = ('Resistor' if ref.startswith('R') else 'Capacitor' if ref.startswith('C') else 'Diode / indicator' if ref.startswith('D') else 'Mounting hole' if ref.startswith('Mounting') else c['value'])
 title,category,description,caution=notes.get(ref,(fallback,'Mechanical' if ref.startswith('Mounting') else 'Passives',c['properties'].get('Description',''),''))
 if ref not in notes and ref.startswith(('R','C')):
  ns=list(dict.fromkeys(p['net'][1] for p in c['pads'] if len(p['net'])>1))
  description = (f"{c['value']} {fallback.lower()} between " + ' and '.join(ns)+'. Exact role follows the surrounding circuit; select a pad to inspect connected parts.')
 angle=math.radians(float(c['at'][2]) if len(c['at'])>2 else 0)
 pads=[];seen=set()
 for p in c['pads']:
  if not p['number'] or p['number'] in seen: continue
  seen.add(p['number'])
  px,py=map(float,p['at'][:2])
  net=p['net'][1] if len(p['net'])>1 else ''
  pads.append({'number':p['number'],'net':net,'x':float(c['at'][0])+px*math.cos(angle)+py*math.sin(angle),'z':float(c['at'][1])-px*math.sin(angle)+py*math.cos(angle),'size':[float(x) for x in p['size'][:2]],'peers':by_net.get(net,[])})
  if ref=='J13':pads[-1]['label']={'1':'Touch 1 / Single','2':'Touch 3 / Record','3':'Touch 2 / Menu','4':'Touch 4 / Audition','5':'Touch 5 / FN'}.get(p['number'],'')
 parts.append({'ref':ref,'value':c['value'],'title':title,'category':category,'description':description,'caution':caution,'footprint':c['footprint'],'x':float(c['at'][0]),'z':float(c['at'][1]),'angle':float(c['at'][2]) if len(c['at'])>2 else 0,'side':c['layer'][0],'pads':pads,'datasheet':c['properties'].get('Datasheet',''),'lcsc':c['properties'].get('lcsc#','')})
(HERE/'assets/board-data.json').write_text(json.dumps({'parts':parts,'source':str(SOURCE.relative_to(HERE.parents[1])),'sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'thickness':1.6,'modelExport':'KiCad GLB: components, pads, soldermask and silkscreen; no copper tracks'},separators=(',',':')))
print(f'Prepared {len(parts)} selectable parts and {sum(len(p["pads"]) for p in parts)} unique pads')
