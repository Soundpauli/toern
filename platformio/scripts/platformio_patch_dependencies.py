from pathlib import Path
import re

Import("env")


project_dir = Path(env.subst("$PROJECT_DIR"))
# Framework changes must survive clean installs and apply to both MCP builds.
import runpy
_usb_patch = runpy.run_path(str(project_dir / "scripts" / "patch_usb_audio.py"))["patch_usb_audio"]
_usb_path = Path(env.PioPlatform().get_package_dir("framework-arduinoteensy")) / "cores/teensy4/usb_audio.cpp"
_usb_source = _usb_path.read_text()
_usb_patched = _usb_patch(_usb_source)
if _usb_patched != _usb_source:
    _usb_path.write_text(_usb_patched)

# Temporary stall diagnostics; keep the flashed diagnostic build reproducible.
_audio_patch = runpy.run_path(str(project_dir / "scripts" / "patch_audio_timing.py"))["patch_audio_timing"]
_audio_path = _usb_path.with_name("AudioStream.cpp")
_audio_source = _audio_path.read_text()
_audio_patched = _audio_patch(_audio_source)
if _audio_patched != _audio_source:
    _audio_path.write_text(_audio_patched)

libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")


def replace_once(text, old, new, label):
    if old not in text:
        raise RuntimeError(f"Cannot apply PlatformIO dependency patch: {label}")
    return text.replace(old, new, 1)


# DuPPa stores ISTEP as a big-endian 32-bit integer. The pinned library's
# readStep reads only its high 16 bits, reporting zero for the configured step 1.
for encoder_dir in libdeps_dir.glob("DuPPa Library*"):
    encoder_source = encoder_dir / "src" / "i2cEncoderLibV2.cpp"
    text = encoder_source.read_text()
    old = "return (readEncoderInt(REG_ISTEPB4));"
    new = "return (readEncoderLong(REG_ISTEPB4));"
    if new not in text:
        text = replace_once(text, old, new, "DuPPa 32-bit step readback")
        encoder_source.write_text(text)

    # Never dispatch 0xFF as every encoder event after a failed status read.
    # Decode counters only after a complete register read; short reads must not
    # turn Wire.read() == -1 into a new knob position.
    replacements = {
        "uint8_t i2cEncoderLibV2::readEncoderByte(uint8_t reg)": r"""uint8_t i2cEncoderLibV2::readEncoderByte(uint8_t reg) {
  const uint8_t failure=(reg==REG_ESTATUS||reg==REG_I2STATUS)?0:0xFF;
  Wire.beginTransmission(_add);Wire.write(reg);
  if(Wire.endTransmission()!=0)return failure;
  if(Wire.requestFrom(_add,(uint8_t)1)!=1||Wire.available()<1)return failure;
  return (uint8_t)Wire.read();
}""",
        "int16_t i2cEncoderLibV2::readEncoderInt(uint8_t reg)": r"""int16_t i2cEncoderLibV2::readEncoderInt(uint8_t reg) {
  Wire.beginTransmission(_add);Wire.write(reg);
  if(Wire.endTransmission()!=0)return (int16_t)_tem_data.val;
  if(Wire.requestFrom(_add,(uint8_t)2)!=2||Wire.available()<2)return (int16_t)_tem_data.val;
  _tem_data.bval[1]=Wire.read();_tem_data.bval[0]=Wire.read();
  return (int16_t)_tem_data.val;
}""",
    }
    for signature, replacement in replacements.items():
        start=text.index(signature);end=text.index("{",start)+1;depth=1
        while depth:
            depth+=(text[end]=="{")-(text[end]=="}");end+=1
        text=text[:start]+replacement+text[end:]
    encoder_source.write_text(text)


for library_dir in libdeps_dir.glob("TeensyAudioSampler*"):
    for controller in (library_dir / "src").glob("**/sampleplaymidicontroller.h"):
        text = controller.read_text()
        patched = re.sub(r"\bbyte\b", "uint8_t", text)
        if patched != text:
            controller.write_text(patched)

    sampler = library_dir / "src" / "sampler.h"
    text = sampler.read_text()
    compatibility_signature = (
        "void noteEvent(uint8_t noteNumber, uint8_t velocity, "
        "bool isNoteOn, bool retrigger)"
    )
    if compatibility_signature not in text:
        marker = """    void addSample(TSampleType *sample) {"""
        overload = """    void noteEvent(uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {
        noteEvent(noteNumber, 0, velocity, isNoteOn, retrigger);
    }

    void addSample(TSampleType *sample) {"""
        text = replace_once(text, marker, overload, "legacy noteEvent overload")

    # Preserve the behavior of the manually modified Arduino library.
    text = re.sub(
        r"^(\s*)audio_voice->_audiomixer->gain\( audio_voice->_mixerChannel, velocity / 255\.0\);",
        r"\1// audio_voice->_audiomixer->gain( audio_voice->_mixerChannel, velocity / 255.0);",
        text,
        flags=re.MULTILINE,
    )
    text = re.sub(
        r"^(\s*)audio_voice->_audiomixer2->gain\(audio_voice->_mixerChannel, velocity / 255\.0\);",
        r"\1// audio_voice->_audiomixer2->gain(audio_voice->_mixerChannel, velocity / 255.0);",
        text,
        flags=re.MULTILINE,
    )
    # Respect release configured by TŒRN; do not replace it with a fixed 2 ms.
    text = text.replace("audio_voice->_audioenvelop->release(2);", "")
    text = text.replace("audio_voice->_audioenvelop2->release(2);", "")
    sampler.write_text(text)

    polyphonic_sampler = library_dir / "src" / "polyphonicsampler.h"
    text = polyphonic_sampler.read_text()
    text = re.sub(
        r'^(\s*)Serial\.printf\("Voice %i plays note %i \(%i, %i\)\\n", indexOfVoice, noteNumber, velocity, isretrigger\);',
        r'\1// Serial.printf("Voice %i plays note %i (%i, %i)\\n", indexOfVoice, noteNumber, velocity, isretrigger);',
        text,
        flags=re.MULTILINE,
    )
    # No USB Serial waits inside the sampler transaction, including rare paths.
    text = re.sub(r'^(\s*)(Serial\.printf\([^\n]*\);)', r'\1// \2', text, flags=re.MULTILINE)
    polyphonic_sampler.write_text(text)


playback_src_dirs = list(libdeps_dir.glob("TeensyVariablePlayback@src-*/src"))
plain_playback_src = libdeps_dir / "TeensyVariablePlayback" / "src"
if plain_playback_src.is_dir() and not playback_src_dirs:
    playback_src_dirs.append(plain_playback_src)

for source_dir in playback_src_dirs:
    (source_dir / "toern_pcm_activity.h").write_text(
        (project_dir / "src" / "toern_pcm_activity.h").read_text()
    )
    (source_dir / "toern_pcm_interpolation.h").write_text(
        (project_dir / "src" / "toern_pcm_interpolation.h").read_text()
    )
    (source_dir / "toern_wsola.h").write_text(
        (project_dir / "src" / "toern_wsola.h").read_text()
    )
    reader = source_dir / "ResamplingReader.h"
    reader.write_text((project_dir / "src" / "resamplerReader.h").read_text())

    playresmp = source_dir / "playresmp.h"
    text = playresmp.read_text()
    if "void setAmplitude(float n)" not in text:
        text = replace_once(
            text,
            "AudioPlayResmp(): AudioStream(0, NULL), reader(nullptr)",
            "AudioPlayResmp(): AudioStream(0, NULL), reader(nullptr), ampMult(65536), ampTarget(65536)",
            "AudioPlayResmp constructor",
        )
        text = replace_once(
            text,
            """        void update()
        {
            int _numChannels = reader->getNumChannels();""",
            """        // 0..1 playback level (note velocity x fader), ramped per sample.
        void setAmplitude(float n) {
            if (n < 0.0f) n = 0.0f;
            else if (n > 1.0f) n = 1.0f;
            int32_t m = (int32_t)(n * 65536.0f + 0.5f);
            ampTarget = m;
            if (reader == nullptr || !reader->isPlaying()) ampMult = m;
        }

        void snapAmplitude(float n) {
            if (n < 0.0f) n = 0.0f;
            else if (n > 1.0f) n = 1.0f;
            int32_t m = (int32_t)(n * 65536.0f + 0.5f);
            ampTarget = m;
            ampMult = m;
        }

        void update()
        {
            int _numChannels = reader->getNumChannels();""",
            "setAmplitude method",
        )
    elif "void snapAmplitude(float n)" not in text:
        text = replace_once(
            text,
            """        void update()
        {
            int _numChannels = reader->getNumChannels();""",
            """        void snapAmplitude(float n) {
            if (n < 0.0f) n = 0.0f;
            else if (n > 1.0f) n = 1.0f;
            int32_t m = (int32_t)(n * 65536.0f + 0.5f);
            ampTarget = m;
            ampMult = m;
        }

        void update()
        {
            int _numChannels = reader->getNumChannels();""",
            "snapAmplitude method",
        )
    # Apply the ramp and its fields on fresh installs as well as older patches.
    if "int32_t cur = ampMult;" not in text:
        text = replace_once(
            text,
            """            unsigned int i, n;
            audio_block_t *blocks[_numChannels];
            int16_t *data[_numChannels];
            // only update if we're playing
            if (!reader->isPlaying()) return;""",
        """            unsigned int n;
            audio_block_t *blocks[_numChannels];
            int16_t *data[_numChannels];
            if (!reader->isPlaying()) {
                ampMult = ampTarget;
                return;
            }
            int32_t cur = ampMult;
            const int32_t tgt = ampTarget;""",
            "amplitude state",
        )
        text = replace_once(
            text,
            """                n = reader->read((void**)data, AUDIO_BLOCK_SAMPLES);
                for (int channel=0; channel < _numChannels; channel++) {""",
            """                n = reader->read((void**)data, AUDIO_BLOCK_SAMPLES);
                const int32_t delta = tgt - cur;
                int32_t step = delta / (int32_t)AUDIO_BLOCK_SAMPLES;
                if (delta != 0 && step == 0) step = (delta > 0) ? 1 : -1;
                if (cur != 65536 || tgt != 65536 || step != 0) {
                    for (unsigned int s = 0; s < n; s++) {
                        if (step != 0) {
                            cur += step;
                            if ((step > 0 && cur > tgt) || (step < 0 && cur < tgt)) {
                                cur = tgt;
                                step = 0;
                            }
                        }
                        for (int channel=0; channel < _numChannels; channel++)
                            data[channel][s] = (int16_t)(((int32_t)data[channel][s] * cur) >> 16);
                    }
                }
                ampMult = cur;
                for (int channel=0; channel < _numChannels; channel++) {""",
            "amplitude ramp",
        )
        text = replace_once(
            text,
            """    protected:
        TResamplingReader *reader;""",
            """    protected:
        TResamplingReader *reader;
        volatile int32_t ampMult;
        volatile int32_t ampTarget;""",
            "amplitude members",
        )

    if not re.search(r"reader->close\(\);\s+(?:pcmActivity\.clear\(\);\s+)?ampMult = ampTarget;", text):
        text = replace_once(
            text,
            """            } else {
                reader->close();
            }""",
            """            } else {
                reader->close();
                ampMult = ampTarget;
            }""",
            "amplitude reset after playback",
        )
    if "void setTimeStretch(bool enabled)" not in text:
        text = replace_once(text, "        void setPlaybackRate(float f) {",
            "        void setTimeStretch(bool enabled) { reader->setTimeStretch(enabled); }\n\n        void setPlaybackRate(float f) {",
            "WSOLA enable forwarding")
    if "void setTimeStretchAmount(uint8_t value)" not in text:
        text = replace_once(text, "        void setPlaybackRate(float f) {",
            "        void setTimeStretchAmount(uint8_t value) { reader->setTimeStretchAmount(value); }\n\n        void setPlaybackRate(float f) {",
            "WSOLA length forwarding")
    if "bool hasRecentOutput()" not in text:
        text = replace_once(text, '#include "loop_type.h"',
            '#include "loop_type.h"\n#include "toern_pcm_activity.h"', 'PCM activity include')
        text = replace_once(text, '        void stop() {\n            reader->stop();',
            '        bool hasRecentOutput() const { return pcmActivity.active(); }\n\n'
            '        void stop() {\n            pcmActivity.clear();\n            reader->stop();',
            'PCM activity accessor/stop')
        text = replace_once(text, '            if (!reader->isPlaying()) {\n                ampMult = ampTarget;',
            '            if (!reader->isPlaying()) {\n                pcmActivity.clear();\n                ampMult = ampTarget;',
            'PCM activity idle')
        text = replace_once(text, '                ampMult = cur;\n                for (int channel=0;',
            '                ampMult = cur;\n                pcmActivity.observe(data, _numChannels, n);\n                for (int channel=0;',
            'PCM activity rendered blocks')
        text = replace_once(text, '                reader->close();\n                ampMult = ampTarget;',
            '                reader->close();\n                pcmActivity.clear();\n                ampMult = ampTarget;',
            'PCM activity EOF')
        text = replace_once(text, '        volatile int32_t ampTarget;',
            '        volatile int32_t ampTarget;\n        ToernPcmActivity pcmActivity;',
            'PCM activity member')
    playresmp.write_text(text)

# Stock AudioEffectDelayExternal talks SPI RAM (23LC1024). TŒRN uses a PSRAM
# copy in src/effect_delay_ext.cpp — skip the library object to avoid dupes.
def _skip_stock_delay_ext(node):
    path = str(node)
    if "effect_delay_ext.cpp" in path and "src/effect_delay_ext.cpp" not in path.replace("\\", "/"):
        return None
    return node

env.AddBuildMiddleware(_skip_stock_delay_ext)
