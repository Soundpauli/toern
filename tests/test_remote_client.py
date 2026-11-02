"""Serial framing, error recovery, input sequencing and PNG tests without a device."""
from pathlib import Path
import collections
import json
import struct
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'standalone-tools/toern-mcp'))
from remote import ToernRemote, RemoteError, DeviceRejected, display_png


class FakeSerial:
    def __init__(self, **kwargs):
        self.options = kwargs
        self.replies = collections.deque()
        self.commands = []
        self.closed = False
        self.protocol = 1
        self.silent = False
        self.reject = None
        self.fragment = False
        self.buttons = [False]*4
        self.touches = [False]*5
        self.pending = False

    def write(self, data):
        words = data.decode().split()
        self.commands.append(words[2:])
        if self.silent:
            return len(data)
        op = words[2]
        if op in ('BUTTON', 'TOUCH'):
            (self.buttons if op == 'BUTTON' else self.touches)[int(words[3])-1] = bool(int(words[4]))
        if op in ('RELEASE', 'BYE'):
            self.buttons[:] = [False]*4
            self.touches[:] = [False]*5
            self.pending = False
        if op == 'TURN':
            self.pending = True
        elif op == 'STATE':
            self.pending = False
        reply = dict(id=int(words[1]), ok=True, protocol=self.protocol,
                     pending_turns=self.pending, buttons=self.buttons, remote_touches=self.touches)
        if self.reject:
            reply.update(ok=False, error=self.reject)
        self.replies.append(b'boot diagnostic\n')
        self.replies.append(b'REMOTE {"id":0,"ok":true}\n')
        encoded = b'REMOTE ' + json.dumps(reply).encode() + b'\n'
        if self.fragment:
            self.replies.extend([encoded[:10], encoded[10:]])
        else:
            self.replies.append(encoded)
        return len(data)

    def read_until(self, *_):
        return self.replies.popleft() if self.replies else b''

    def close(self):
        self.closed = True


class ClientTests(unittest.TestCase):
    def setUp(self):
        self.wire = FakeSerial()
        self.client = ToernRemote(serial_factory=lambda **kw: self.wire, timeout=0.15)
        self.client.connect('fake')

    def tearDown(self):
        self.client.disconnect()

    def test_fragmented_frames_and_other_ids(self):
        self.wire.fragment = True
        self.assertTrue(self.client.get_state()['ok'])

    def test_turn_waits_for_input_poll(self):
        self.assertFalse(self.client.turn_encoder(4, 3)['pending_turns'])
        self.assertIn(['TURN', '4', '3'], self.wire.commands)
        self.assertGreaterEqual(self.wire.commands.count(['STATE']), 2)

    def test_device_rejection_is_recoverable(self):
        self.wire.reject = 'BUSY'
        with self.assertRaises(DeviceRejected):
            self.client.get_state()
        self.assertFalse(self.wire.closed)
        self.wire.reject = None
        self.assertTrue(self.client.get_state()['ok'])

    def test_timeout_closes_and_never_retries(self):
        self.wire.silent = True
        before = len(self.wire.commands)
        with self.assertRaises(TimeoutError):
            self.client.request('TURN', 1, 1)
        self.assertEqual(len(self.wire.commands), before+1)
        self.assertTrue(self.wire.closed)
        self.assertIsNone(self.client.serial)

    def test_validation_precedes_serial_write(self):
        before = len(self.wire.commands)
        for args in [(0, 1), (5, 1), (1, 65), (1, 0), (True, 1)]:
            with self.assertRaises(ValueError):
                self.client.turn_encoder(*args)
        with self.assertRaises(ValueError):
            self.client.gesture([1], [5], 2500)
        with self.assertRaises(ValueError):
            self.client.request('TURN\nRM /', 1, 1)
        self.assertEqual(len(self.wire.commands), before)

    def test_gesture_fn_order_and_release(self):
        self.client.gesture([2], [5], 80, encoder=1, steps=-1)
        cmds = self.wire.commands
        self.assertLess(cmds.index(['TOUCH','5','1']), cmds.index(['BUTTON','2','1']))
        self.assertLess(cmds.index(['BUTTON','2','0']), cmds.index(['TOUCH','5','0']))
        self.assertFalse(any(self.wire.buttons+self.wire.touches))

    def test_gesture_failure_releases_controls(self):
        original = self.client.turn_encoder
        def fail(*_):
            raise RemoteError('test failure')
        self.client.turn_encoder = fail
        with self.assertRaises(RemoteError):
            self.client.gesture([2], [5], 80, encoder=1)
        self.client.turn_encoder = original
        self.assertFalse(any(self.wire.buttons+self.wire.touches))

    def test_gesture_does_not_release_existing_hold(self):
        self.wire.buttons[0] = True
        with self.assertRaises(RemoteError):
            self.client.gesture([2], [], 80)
        self.assertTrue(self.wire.buttons[0])

    def test_display_waits_for_first_presented_frame(self):
        from unittest.mock import patch, call
        frame = {'width': 1, 'height': 1, 'rgb': '123456'}
        with patch.object(self.client, 'request', side_effect=[DeviceRejected('NO_FRAME'), frame]) as request:
            self.assertEqual(self.client.get_display_frame(), frame)
            self.assertEqual(request.call_args_list, [call('DISPLAY'), call('DISPLAY')])
        with patch.object(self.client, 'request', side_effect=DeviceRejected('NO_SESSION')) as request:
            with self.assertRaisesRegex(DeviceRejected, 'NO_SESSION'):
                self.client.get_display_frame()
            request.assert_called_once_with('DISPLAY')

    def test_disabled_build_connect_reports_error_and_closes(self):
        self.client.disconnect()
        self.wire.reject = 'DISABLED'
        with self.assertRaisesRegex(DeviceRejected, 'DISABLED'):
            self.client.connect('fake')
        self.assertTrue(self.wire.closed)
        self.assertIsNone(self.client.serial)

    def test_disconnect_releases(self):
        self.client.set_control('touch', 5, True)
        self.client.disconnect()
        self.assertEqual(self.wire.commands[-1], ['BYE'])
        self.assertTrue(self.wire.closed)

    def test_connect_timeout_preserves_original_error(self):
        self.client.disconnect()
        self.wire.silent = True
        with self.assertRaises(TimeoutError):
            self.client.connect('fake')
        self.assertTrue(self.wire.closed)
        self.assertIsNone(self.client.serial)

    def test_protocol_mismatch_closes(self):
        self.client.disconnect()
        self.wire.closed = False
        self.wire.protocol = 99
        with self.assertRaises(RemoteError):
            self.client.connect('fake')
        self.assertTrue(self.wire.closed)
        self.assertIsNone(self.client.serial)

    def test_png_pixels_dimensions_and_crc(self):
        png = display_png({'width': 2, 'height': 1, 'rgb': 'ff000000ff00'}, scale=2)
        self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
        offset, compressed = 8, b''
        while offset < len(png):
            size, = struct.unpack('>I', png[offset:offset+4])
            kind = png[offset+4:offset+8]
            data = png[offset+8:offset+8+size]
            crc, = struct.unpack('>I', png[offset+8+size:offset+12+size])
            self.assertEqual(crc, zlib.crc32(kind+data) & 0xffffffff)
            if kind == b'IHDR':
                self.assertEqual(struct.unpack('>II', data[:8]), (4, 2))
            if kind == b'IDAT':
                compressed += data
            offset += 12+size
        row = b'\0'+bytes.fromhex('ff0000ff000000ff0000ff00')
        self.assertEqual(zlib.decompress(compressed), row*2)
        with self.assertRaises(RemoteError):
            display_png({'width': 16, 'height': 16, 'rgb': 'ff'})


if __name__ == '__main__':
    unittest.main()
