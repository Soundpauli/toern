"""Record and check real USB playback; no synthetic audio substituted for capture.

Requires requirements-audio-test.txt. Only --full-reset performs a global reset.
Filter changes and the temporary sequencer pattern are restored in finally.
Default hardware coverage is CH1/CH2; other sample voices use the same checks.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import time
import zlib

import numpy as np
from scipy import signal
import soundfile as sf

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / 'tools/toern-mcp'))
from remote import ToernRemote, DeviceRejected

# Hardware clients are shared with the standalone tools.
sys.path.insert(0, str(REPO / 'tools/sdtool'))
from toern_sd import ToernSd, read_line

RATE = 44100
def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))


def rms(data):
    return float(np.sqrt(np.mean(np.asarray(data, dtype=np.float64) ** 2)))


def match_reference(recording, reference):
    """Align losslessly at integer sample offset, then fit a single level gain.

    No time warping, denoising or per-window gain fitting: those could hide
    dropouts, envelope faults or pitch errors. Check each USB channel separately.
    """
    y = np.asarray(recording, dtype=np.float64)
    x = np.asarray(reference, dtype=np.float64)
    if y.ndim == 1:
        y = y[:, None]
    if len(x) < 16 or rms(x) < 1e-6 or len(y) < len(x):
        raise ValueError('Reference must contain audible PCM and fit in the capture')
    correlation = signal.correlate(y[:, 0], x, mode='valid', method='fft')
    offset = int(np.argmax(np.abs(correlation)))
    segment = y[offset:offset + len(x)]
    gain = np.sum(segment * x[:, None], axis=0) / np.sum(x*x)
    expected = x[:, None] * gain
    residual = segment - expected
    snr = 20*np.log10(np.maximum(np.sqrt(np.mean(expected**2, axis=0)), 1e-12)
                       / np.maximum(np.sqrt(np.mean(residual**2, axis=0)), 1e-12))
    # Largest added discontinuity after subtracting the actual WAV transient.
    added_jump = float(np.max(np.abs(np.diff(residual, axis=0))))
    threshold = max(1e-5, rms(expected)*0.02)
    window = 128
    missing = 0
    for start in range(0, len(x)-window+1, window):
        a = rms(expected[start:start+window])
        b = rms(segment[start:start+window])
        if a > threshold and b < a*0.05:
            missing += 1
    return dict(offset_samples=offset, gain=gain.tolist(), snr_db=snr.tolist(),
                added_jump_dbfs=db(added_jump), missing_blocks=missing,
                residual_rms_dbfs=db(rms(residual)))


def envelope_reference(reference, attack=0.0, decay=1.0, sustain=1.0):
    """Ideal sample ADS curve; device also has sub-ms block/8-sample rounding.

    The stock Teensy envelope holds its peak for 2.5ms. Release is tested with
    an explicit panel mute/note-off, not inferred from natural sample decay.
    """
    t = np.arange(len(reference)) / RATE
    env = np.ones(len(t))
    if attack:
        env = np.minimum(t/attack, 1.0)
    after = t - attack - 0.0025
    if decay:
        env = np.where(after > 0, np.maximum(sustain, 1-(1-sustain)*after/decay), env)
    else:
        env = np.where(after > 0, sustain, env)
    return np.asarray(reference)*env


def default_reference(reference):
    # Teensy's stock attack(0) still has one eight-sample ramp (0.181ms).
    # Model that known onset for added-click detection; independently report
    # and check SNR against the unmodified WAV, including its ENTIRE tail.
    expected = np.asarray(reference).copy()
    count = min(8, len(expected))
    expected[:count] *= np.arange(1, count+1)/8
    return expected


def match_release(recording, reference, seconds):
    """Allow panel timing uncertainty, but keep the requested release slope fixed."""
    t = np.arange(len(reference))/RATE
    best = None
    for gate in np.arange(.025, .151, .001):
        expected = reference*np.clip(1-(t-gate)/seconds, 0, 1)
        candidate = match_reference(recording, expected)
        if best is None or min(candidate['snr_db']) > min(best['snr_db']):
            best = dict(candidate, gate_ms=float(gate*1000), requested_release_ms=seconds*1000)
    return best


def read_exact(serial, count):
    data = bytearray()
    deadline = time.monotonic()+15
    while len(data) < count:
        data.extend(serial.read(count-len(data)))
        if time.monotonic() > deadline:
            raise TimeoutError('Binary serial read stalled')
    return bytes(data)


def get_pattern(remote):
    with remote.lock:
        serial = remote.serial
        serial.write(b'GETPAT\n')
        header = read_line(serial)
        if not header.startswith('OK '):
            raise RuntimeError(header)
        size = int(header.split()[1])
        if size != 20494:
            raise RuntimeError(f'Unsupported pattern size: {size}')
        serial.write(b'ACK\n')
        data = bytearray()
        while len(data) < size:
            data.extend(read_exact(serial, min(512, size-len(data))))
            serial.write(b'ACK\n')
        if read_line(serial) != f'CRC {zlib.crc32(data):08X}':
            raise RuntimeError('Pattern CRC mismatch')
        return bytes(data)


def put_pattern(remote, data):
    with remote.lock:
        serial = remote.serial
        serial.write(f'PUTPAT {len(data)} {zlib.crc32(data):08X}\n'.encode())
        if read_line(serial) != 'READY':
            raise RuntimeError('PUTPAT not ready')
        for start in range(0, len(data), 8192):
            serial.write(data[start:start+8192])
            serial.flush()
            if read_line(serial) != 'ACK':
                raise RuntimeError('PUTPAT missing ACK')
        if read_line(serial) != 'OK':
            raise RuntimeError('PUTPAT failed')


class Panel:
    def __init__(self, remote):
        self.r = remote

    def press(self, kind, index, ms=120):
        return self.r.gesture([index] if kind == 'button' else [],
                              [index] if kind == 'touch' else [], ms)

    def position(self, encoder, target):
        state = self.r.get_state()
        for _ in range(16):
            delta = target-state['positions'][encoder-1]
            if not delta:
                return state
            state = self.r.turn_encoder(encoder, max(-64, min(64, delta)))
        raise RuntimeError('Encoder failed to reach target')

    def draw(self):
        for _ in range(6):
            state = self.r.get_state()
            if state['mode'] == 'DRAW':
                return state
            self.press('touch', 1 if state['mode'] in ('SINGLE', 'FILTERMODE') else 2)
        raise RuntimeError('Cannot reach DRAW')

    def select(self, voice):
        self.draw()
        self.position(1, voice+1)

    def set_values(self, voice, **values):
        self.select(voice)
        state = self.press('button', 3, 500)
        if state['mode'] != 'FILTERMODE' or state['voice'] != voice:
            raise RuntimeError(f'Cannot enter filters: {state}')
        try:
            # INST first because selecting a preset rewrites other settings.
            for name in sorted(values, key=lambda x: x != 'INST'):
                data = self.r.request('FILTERS', voice)
                page, slider = next((p, s) for p in data['pages'] for s in p['sliders'] if s['name'] == name)
                for _ in range(len(data['pages'])):
                    if data['active_page'] == page['page']:
                        break
                    self.press('touch', 2)
                    data = self.r.request('FILTERS', voice)
                delta = values[name]-slider['value']
                while delta:
                    step = max(-64, min(64, delta))
                    self.r.turn_encoder(slider['encoder'], step)
                    delta -= step
            actual = {s['name']:s['value'] for p in self.r.request('FILTERS', voice)['pages'] for s in p['sliders']}
            if any(actual[k] != v for k, v in values.items()):
                raise RuntimeError(f'Filter readback failed: {values} vs {actual}')
        finally:
            self.draw()

    def full_reset(self):
        self.draw()
        self.press('touch', 2)
        menu = self.r.request('MENUS')
        self.position(4, next(i['index'] for i in menu['items'] if i['name'] == 'ETC'))
        self.press('button', 4)
        menu = self.r.request('MENUS')
        self.position(4, next(i['index'] for i in menu['items'] if i['name'] == 'RSET'))
        self.position(3, 2)
        state = self.r.get_state()
        if state['menu_setting'] != 16 or state['positions'][2] != 2:
            raise RuntimeError('RSET FULL selector not confirmed')
        try:
            self.press('button', 2)
        except DeviceRejected as error:
            if str(error) != 'NO_SESSION':
                raise
            # FULL can scan SD for longer than the remote lease. Re-handshake
            # after the completed reset; never replay the destructive action.
            self.r.connect(self.r.port)
        self.draw()
        state = self.r.get_state()
        if state['kit'] != 1 or state['playing'] or state['cell']['voice']:
            raise RuntimeError(f'FULL reset did not reach its expected state: {state}')


class Recorder:
    def __init__(self, device):
        import sounddevice as sd
        self.sd = sd
        candidates = [i for i, d in enumerate(sd.query_devices())
                      if d['max_input_channels'] >= 2 and 'teensy' in d['name'].lower()]
        if device is None:
            if len(candidates) != 1:
                raise RuntimeError(f'Specify --audio-device; Teensy inputs: {candidates}')
            device = candidates[0]
        info = sd.query_devices(device, 'input')
        if 'teensy' not in info['name'].lower():
            raise RuntimeError('Refusing to record an unrelated input/microphone')
        sd.check_input_settings(device=device, channels=2, dtype='float32', samplerate=RATE)
        self.device, self.info = device, dict(info)
        self.remote = None
        self.performance = []

    def capture(self, seconds, action=None):
        blocks, statuses = [], []
        self.performance = []
        def callback(data, frames, times, status):
            if status:
                statuses.append(str(status))
            blocks.append(data.copy())
        with self.sd.InputStream(device=self.device, samplerate=RATE, channels=2,
                                 dtype='float32', callback=callback):
            time.sleep(.3)
            if action:
                action()
            deadline = time.monotonic()+seconds
            while time.monotonic() < deadline:
                if self.remote:
                    self.performance.append(self.remote.request('PERF'))
                time.sleep(min(.1, max(0, deadline-time.monotonic())))
        if not blocks:
            raise RuntimeError('USB input produced no audio blocks')
        return np.concatenate(blocks), statuses


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--channels', default='1,2', help='Hardware sample voices to test, 1..8')
    parser.add_argument('--kit', type=int, default=1)
    parser.add_argument('--audio-device', type=int)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--full-reset', action='store_true', help='Apply RSET FULL before testing (clears pattern/settings)')
    parser.add_argument('--reverb', action='store_true', help='Also record maximum reverb and bypass recovery')
    parser.add_argument('--reference', action='append', default=[], metavar='VOICE=FILE',
                        help='Compare an already-loaded custom voice to this local WAV instead of a kit slot')
    args = parser.parse_args()
    channels = list(dict.fromkeys(int(ch) for ch in args.channels.split(',')))
    if not channels or any(ch < 1 or ch > 8 for ch in channels):
        parser.error('This reference-WAV suite tests sample voices 1..8; synths need a separate stimulus')
    overrides = {}
    for item in args.reference:
        voice, separator, path = item.partition('=')
        if not separator or not voice.isdigit() or int(voice) not in channels or not Path(path).is_file():
            parser.error('--reference requires a tested VOICE=existing-WAV-file')
        overrides[int(voice)] = Path(path)
    if args.full_reset and overrides:
        parser.error('FULL reloads kit 1; load custom voices after resetting, then run without --full-reset')
    args.output.mkdir(parents=True, exist_ok=True)
    recorder = Recorder(args.audio_device)
    remote = ToernRemote(timeout=20)
    panel = Panel(remote)
    report = dict(sample_rate=RATE, input=recorder.info, cases=[], channels=channels,
                  scope='USB digital output; sample voices only. No analog-output or synth certification.',
                  full_reset=args.full_reset)
    def save():
        (args.output/'report.json').write_text(json.dumps(report, indent=2))
    def case(name, audio, statuses, metrics, checks):
        sf.write(args.output/f'{name}.wav', audio, RATE, subtype='PCM_24')
        if recorder.performance:
            metrics = dict(metrics, cpu_peak_percent=max(p['cpu_bp'] for p in recorder.performance)/100,
                           cpu_mean_percent=float(np.mean([p['cpu_bp'] for p in recorder.performance]))/100)
        checks = dict(checks, usb_capture_clean=not statuses, finite=bool(np.isfinite(audio).all()))
        result = dict(name=name, metrics=metrics, checks=checks, capture_status=statuses,
                      passed=all(checks.values()))
        report['cases'].append(result)
        print(('PASS' if result['passed'] else 'FAIL'), name, json.dumps(metrics), flush=True)
        save()
    originals, pattern, initial = {}, None, None
    try:
        initial = remote.connect(args.port)
        report['firmware_performance_at_start'] = remote.request('PERF')
        if args.full_reset:
            panel.full_reset()
            initial = remote.get_state()
        if initial['kit'] != args.kit:
            raise RuntimeError(f'Load kit {args.kit} first; loaded kit is {initial["kit"]}')
        originals = {ch: {s['name']:s['value'] for p in remote.request('FILTERS', ch)['pages'] for s in p['sliders']}
                     for ch in channels}
        pattern = get_pattern(remote)  # Holds current pattern in process RAM only; stops playback.
        remote.disconnect()
        sd = ToernSd(args.port)
        try:
            references = {}
            for ch in channels:
                source = str(overrides[ch].resolve()) if ch in overrides else f'/{args.kit}/{ch}.wav'
                data = overrides[ch].read_bytes() if ch in overrides else sd.get_bytes(source)
                (args.output/f'reference-ch{ch}.wav').write_bytes(data)
                pcm, rate = sf.read(io.BytesIO(data))
                if rate != RATE or pcm.ndim != 1 or sf.info(io.BytesIO(data)).subtype != 'PCM_16':
                    raise RuntimeError('Reference must be 44.1kHz mono PCM16 WAV')
                references[ch] = pcm
                report.setdefault('sources', {})[ch] = dict(path=source,
                      sha256=hashlib.sha256(data).hexdigest(), frames=len(pcm), peak_dbfs=db(max(abs(pcm))))
                if ch in overrides:
                    # Individual sample loading saves its actual buffer in SP0.
                    # Verify every source frame, including sub-audible tails;
                    # appended zeros are allowed only AFTER the full original.
                    loaded_data = sd.get_bytes(f'/0/{ch}.wav')
                    loaded, loaded_rate = sf.read(io.BytesIO(loaded_data))
                    exact = (loaded_rate == RATE and loaded.ndim == 1 and len(loaded) >= len(pcm)
                             and np.array_equal(loaded[:len(pcm)], pcm)
                             and not np.any(loaded[len(pcm):]))
                    report.setdefault('loader_readback', {})[ch] = dict(pcm_prefix_exact=bool(exact),
                          source_frames=len(pcm), loaded_frames=len(loaded),
                          appended_zero_frames=len(loaded)-len(pcm), path=f'/0/{ch}.wav')
                    if not exact:
                        raise RuntimeError(f'CH{ch} loaded PCM differs from the full original WAV')
        finally:
            sd.close()
        remote.connect(args.port)
        recorder.remote = remote
        panel.draw()
        quiet, status = recorder.capture(.7)
        case('idle', quiet, status, dict(rms_dbfs=db(rms(quiet))), dict(quiet=rms(quiet) < 1e-4))
        for ch in channels:
            reference = references[ch]
            panel.set_values(ch, **{s['name']:s['default'] for p in remote.request('FILTERS', ch)['pages'] for s in p['sliders']})
            # Default A=0, D=1s, S=1, R=0 is a transparent one-shot.
            for repeat in range(3):
                audio, status = recorder.capture(len(reference)/RATE+.45, lambda: panel.press('touch', 4))
                raw = match_reference(audio, reference)
                metrics = match_reference(audio, default_reference(reference))
                metrics['source_snr_db'] = raw['snr_db']
                metrics['native_attack_samples'] = 8
                metrics.update(peak_dbfs=db(np.max(abs(audio))), tail_rms_dbfs=db(rms(audio[-int(.2*RATE):])))
                checks = dict(reference_match=min(metrics['snr_db']) > 35 and min(raw['snr_db']) > 35,
                              unity_level=all(.95 < g < 1.05 for g in metrics['gain']),
                              no_missing_blocks=metrics['missing_blocks'] == 0,
                              no_added_hard_click=metrics['added_jump_dbfs'] < -35,
                              silent_after_sample=metrics['tail_rms_dbfs'] < -80)
                case(f'ch{ch}-dry-{repeat+1}', audio, status, metrics, checks)
            # Fit A/D/S against the *source PCM multiplied by the expected curve*,
            # rather than interpreting a kick/snare's own decay as the ADSR.
            for name, values, shape in (
                ('attack', dict(ATTC=31, DCAY=32, SUST=32, RLSE=0), dict(attack=.0625)),
                ('decay-zero-sustain', dict(ATTC=32, DCAY=2, SUST=0, RLSE=0), dict(decay=.0625, sustain=0)),
                ('decay-half-sustain', dict(ATTC=32, DCAY=2, SUST=16, RLSE=0), dict(decay=.0625, sustain=.5)),
            ):
                panel.set_values(ch, **values)
                time.sleep(.1)
                audio, status = recorder.capture(len(reference)/RATE+.45, lambda: panel.press('touch', 4))
                expected = envelope_reference(reference, **shape)
                metrics = match_reference(audio, expected)
                case(f'ch{ch}-{name}', audio, status, metrics,
                     dict(envelope_shape=min(metrics['snr_db']) > 22,
                          level_preserved=all(.85 < g < 1.15 for g in metrics['gain']),
                          no_missing_blocks=metrics['missing_blocks'] == 0))
            panel.set_values(ch, ATTC=32, DCAY=32, SUST=32, RLSE=0)
            # Explicit note-off via the panel mute action. Touch4 release alone
            # is NOT note-off: sample audition intentionally plays one-shots.
            for raw_release in (1, 3):
                panel.set_values(ch, RLSE=raw_release)
                muted = [False]
                def release_stimulus():
                    remote.request('TOUCH', 4, 1)
                    time.sleep(.025)
                    remote.request('TOUCH', 4, 0)
                    time.sleep(.025)
                    remote.request('BUTTON', 2, 1)
                    time.sleep(.025)
                    remote.request('BUTTON', 2, 0)
                    muted[0] = True
                try:
                    audio, status = recorder.capture(len(reference)/RATE+.45, release_stimulus)
                finally:
                    remote.request('RELEASE')
                    if muted[0]:
                        panel.press('button', 2)
                metrics = match_release(audio, reference, raw_release/32)
                case(f'ch{ch}-release-{raw_release}', audio, status, metrics,
                     dict(release_shape=min(metrics['snr_db']) > 22,
                          level_preserved=all(.85 < g < 1.15 for g in metrics['gain'])))
            panel.set_values(ch, RLSE=0)
            if args.reverb:
                panel.set_values(ch, RVRB=32)
                audio, status = recorder.capture(12, lambda: panel.press('touch', 4))
                early = rms(audio[int(2*RATE):int(4*RATE)])
                late = rms(audio[-2*RATE:])
                metrics = dict(peak_dbfs=db(np.max(abs(audio))), early_tail_dbfs=db(early),
                               late_tail_dbfs=db(late), clipped_fraction=float(np.mean(abs(audio) >= .9999)))
                case(f'ch{ch}-max-reverb', audio, status, metrics,
                     dict(no_hard_clip=metrics['clipped_fraction'] < 1e-4,
                          tail_decays=late < max(early*.25, 1e-5),
                          no_loud_noise_floor=late < .001))
                panel.set_values(ch, RVRB=0)
                quiet, status = recorder.capture(.7)
                case(f'ch{ch}-reverb-bypass', quiet, status, dict(rms_dbfs=db(rms(quiet))),
                     dict(bypass_clears_tail=rms(quiet) < 1e-4))
        # Real sequencer playback, beyond isolated audition: alternating voices
        # every 600ms, with enough space to compare each transient independently.
        if len(channels) == 2:
            test = bytearray(20494)
            test[16384:16386] = b'\xff\xfe'
            struct.pack_into('<f', test, 16386, 100.0)
            test[16390:16398] = b'TPIT\x01\x10\x00\x00'
            test[16398:] = b'\xff'*4096
            for x in range(1, 33, 4):
                ch = channels[((x-1)//4) % 2]
                offset = ((x-1)*16+ch)*4
                test[offset:offset+4] = bytes((ch, 127, 100, 1))
            put_pattern(remote, test)
            panel.draw()
            panel.position(1, 1)
            audio, status = recorder.capture(10, lambda: panel.press('touch', 1))
            # SD's pause(true) avoids autosaving the temporary test sequence.
            # A normal panel pause would persist it before RAM restoration.
            get_pattern(remote)
            metrics, checks = {}, {}
            for ch in channels:
                template = references[ch][:int(.4*RATE)]
                correlation = signal.correlate(audio[:, 0], template, mode='valid', method='fft')
                peaks, _ = signal.find_peaks(np.abs(correlation), height=np.max(abs(correlation))*.65,
                                             distance=int(.8*RATE))
                hits = [match_reference(audio[p:p+len(template)], template) for p in peaks]
                spacing = np.diff(peaks)/RATE
                metrics[f'ch{ch}'] = dict(hits=len(hits),
                      min_snr_db=min((min(h['snr_db']) for h in hits), default=-240),
                      max_spacing_error_ms=float(np.max(abs(spacing-1.2))*1000) if len(spacing) else None)
                checks[f'ch{ch}_repeated_hits'] = len(hits) >= 6
                checks[f'ch{ch}_clean_transients'] = bool(hits) and all(min(h['snr_db']) > 35 for h in hits)
                checks[f'ch{ch}_timing'] = bool(len(spacing)) and bool(np.all(abs(spacing-1.2) < .02))
            case('sequencer-alternating', audio, status, metrics, checks)
        report['passed'] = all(c['passed'] for c in report['cases'])
    except Exception as error:
        report['error'] = str(error)
        report['passed'] = False
        raise
    finally:
        try:
            if pattern is not None:
                if remote.serial is None:
                    remote.connect(args.port)
                put_pattern(remote, pattern)
                if get_pattern(remote) != pattern:
                    raise RuntimeError('Pattern restoration did not round-trip')
                report['pattern_restored'] = True
                for ch, values in originals.items():
                    panel.set_values(ch, **values)
                panel.draw()
                if initial and initial['playing']:
                    panel.position(1, 1)
                    panel.press('touch', 1)
                if initial:
                    panel.position(1, initial['voice']+1)
                    panel.position(4, initial['cursor']['x'])
        except Exception as error:
            report['restore_error'] = str(error)
            report['passed'] = False
        finally:
            remote.disconnect()
            save()
    return 0 if report.get('passed') else 1


if __name__ == '__main__':
    raise SystemExit(main())
