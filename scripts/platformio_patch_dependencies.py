from pathlib import Path
import re

Import("env")


project_dir = Path(env.subst("$PROJECT_DIR"))
libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")


def replace_once(text, old, new, label):
    if old not in text:
        raise RuntimeError(f"Cannot apply PlatformIO dependency patch: {label}")
    return text.replace(old, new, 1)


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
    if "audio_voice->_audioenvelop->release(2);" not in text:
        text = replace_once(
            text,
            "audio_voice->_audioenvelop->noteOff();",
            "audio_voice->_audioenvelop->release(2);\n                    audio_voice->_audioenvelop->noteOff();",
            "voice one envelope release",
        )
    if "audio_voice->_audioenvelop2->release(2);" not in text:
        text = replace_once(
            text,
            "audio_voice->_audioenvelop2->noteOff();",
            "audio_voice->_audioenvelop2->release(2);\n                    audio_voice->_audioenvelop2->noteOff();",
            "voice two envelope release",
        )
    sampler.write_text(text)

    polyphonic_sampler = library_dir / "src" / "polyphonicsampler.h"
    text = polyphonic_sampler.read_text()
    text = re.sub(
        r'^(\s*)Serial\.printf\("Voice %i plays note %i \(%i, %i\)\\n", indexOfVoice, noteNumber, velocity, isretrigger\);',
        r'\1// Serial.printf("Voice %i plays note %i (%i, %i)\\n", indexOfVoice, noteNumber, velocity, isretrigger);',
        text,
        flags=re.MULTILINE,
    )
    polyphonic_sampler.write_text(text)


for source_dir in libdeps_dir.glob("TeensyVariablePlayback@src-*/src"):
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

        void update()
        {
            int _numChannels = reader->getNumChannels();""",
            "setAmplitude method",
        )
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

    if "reader->close();\n                ampMult = ampTarget;" not in text:
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
    playresmp.write_text(text)
