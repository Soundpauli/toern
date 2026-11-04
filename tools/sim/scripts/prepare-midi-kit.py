#!/usr/bin/env python3
"""Render original, deterministic KIT 3 samples (requires numpy).

Run from any directory: python3 tools/sim/scripts/prepare-midi-kit.py
No recordings or third-party soundfonts are used. Melodic roots are MIDI 60.
"""
from pathlib import Path
import wave

import numpy as np

RATE = 44100
ROOT = 261.6255653005986
DEST = Path(__file__).resolve().parents[1] / "SD-CARD-CONTENT" / "3"
rng = np.random.default_rng(303)


def time(seconds):
    return np.arange(round(seconds * RATE)) / RATE


def noise(t, low, high):
    """Band-limited noise with gentle spectral edges."""
    source = rng.normal(size=len(t))
    freq = np.fft.rfftfreq(len(t), 1 / RATE)
    mask = (1 - np.exp(-(freq / low) ** 4)) * np.exp(-(freq / high) ** 4)
    result = np.fft.irfft(np.fft.rfft(source) * mask, n=len(t))
    return result / np.std(result)


def save(voice, sound, peak, fade_in=.001, fade_out=.025):
    sound = sound - np.mean(sound)
    attack = min(len(sound), round(fade_in * RATE))
    release = min(len(sound), round(fade_out * RATE))
    sound[:attack] *= np.linspace(0, 1, attack)
    sound[-release:] *= np.linspace(1, 0, release)
    sound *= peak / np.max(np.abs(sound))
    # Fixed-seed TPDF dither before 16-bit quantization.
    pcm = np.clip(np.rint(sound * 32767 + rng.random(len(sound)) - rng.random(len(sound))), -32768, 32767).astype("<i2")
    pcm[0] = pcm[-1] = 0
    with wave.open(str(DEST / f"{voice}.wav"), "wb") as wav:
        wav.setparams((1, 2, RATE, len(pcm), "NONE", "not compressed"))
        wav.writeframes(pcm.tobytes())


def main():
    DEST.mkdir(parents=True, exist_ok=True)
    # Tight acoustic/electronic hybrid kick: body plus a quiet beater.
    t = time(.55)
    phase = 2 * np.pi * (53 * t + 92 * .018 * (1 - np.exp(-t / .018)))
    kick = np.sin(phase) * np.exp(-t / .12)
    kick += .12 * noise(t, 1200, 6500) * np.exp(-t / .004)
    save(1, np.tanh(1.3 * kick), .84)

    t = time(.42)
    snare = .50 * noise(t, 700, 10500) * np.exp(-t / .075)
    snare += (.42 * np.sin(2 * np.pi * 185 * t) + .18 * np.sin(2 * np.pi * 330 * t)) * np.exp(-t / .045)
    save(2, snare, .73)

    t = time(.18)
    hat = noise(t, 5500, 15500) * np.exp(-t / .028)
    hat += .12 * sum(np.sin(2 * np.pi * f * t) for f in [6320, 8570, 10430]) * np.exp(-t / .017)
    save(3, hat, .43, .0003, .01)

    t = time(.32)
    bursts = sum(np.where(t >= start, np.exp(-np.maximum(t - start, 0) / .009), 0) for start in [0, .011, .023])
    clap = noise(t, 950, 8500) * (bursts + .30 * np.exp(-t / .060))
    save(4, clap, .62)

    t = time(.55)
    phase = 2 * np.pi * (120 * t + 55 * .025 * (1 - np.exp(-t / .025)))
    tom = (np.sin(phase) + .26 * np.sin(1.59 * phase)) * np.exp(-t / .105)
    tom += .08 * noise(t, 1300, 6500) * np.exp(-t / .012)
    save(5, tom, .72)

    # C4 roots are deliberate: the engine transposes all sample voices from 60.
    t = time(2.5)
    bass = sum((1 / h ** 1.65) * np.sin(2 * np.pi * ROOT * h * t) * (.68 + .32 * np.exp(-t / (.14 / h))) for h in range(1, 13))
    bass *= np.exp(-t / 1.7) * (1 - .35 * np.exp(-t / .012))
    save(6, bass, .63, .003, .10)

    t = time(3)
    phase = 2 * np.pi * ROOT * t
    keys = np.sin(phase + 1.35 * np.sin(phase) * np.exp(-t / .22)) * np.exp(-t / 1.25)
    keys += .21 * np.sin(2 * phase) * np.exp(-t / .32)
    keys += .10 * np.sin(4 * phase) * np.exp(-t / .08)
    save(7, keys, .64, .002, .12)

    t = time(4)
    pad = np.zeros_like(t)
    for h in range(1, 18):
        # Broad vowel-like formants; exact harmonics keep the root unambiguous.
        formant = .7 * np.exp(-((ROOT * h - 800) / 380) ** 2) + .3 * np.exp(-((ROOT * h - 1700) / 600) ** 2)
        amp = (1 / h ** 1.6 + formant / h) * (1 + .07 * np.sin(2 * np.pi * (.37 + h * .013) * t))
        pad += amp * np.sin(2 * np.pi * ROOT * h * t + .008 * h * np.sin(2 * np.pi * .48 * t))
    pad *= (1 - np.exp(-t / .035)) * np.exp(-t / 5)
    save(8, pad, .48, .008, .35)
    print(f"Rendered KIT 3: eight mono 44.1 kHz / 16-bit WAVs in {DEST}")


if __name__ == "__main__":
    main()
