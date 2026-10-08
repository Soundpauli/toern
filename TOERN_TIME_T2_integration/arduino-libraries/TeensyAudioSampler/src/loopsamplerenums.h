/* Audio Library for Teensy
 * Copyright (c) 2021, Nic Newdigate
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TEENSY_AUDIO_SAMPLER_LOOPSAMPLER_ENUMS_H
#define TEENSY_AUDIO_SAMPLER_LOOPSAMPLER_ENUMS_H

enum triggertype {
    triggertype_play_until_end = 0,
    triggertype_play_until_subsequent_notedown = 1,
    triggertype_play_while_notedown = 2
};

enum playlooptype {
    playlooptype_once = 0,
    playlooptype_looping = 1,
    playlooptype_pingpong = 2
};

enum playdirection {
    playdirection_begin_forward = 0,
    playdirection_begin_backward = 1
};

#endif // TEENSY_AUDIO_SAMPLER_LOOPSAMPLER_ENUMS_H