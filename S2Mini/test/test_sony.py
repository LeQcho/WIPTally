import io
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from desktop.backend import FlashJobs, images, make_settings, ports, validate
from test_desktop import stored_byte

SONY = dict(role='sony-c3', board='lolin_c3_mini', sony_name='ILCE-7M4',
            sony_mac='', sony_pin=4, sony_pixels=2, rgb_order='RGB', port='SONY-USB')

class SonyTests(unittest.TestCase):
    def test_s2_and_reserved_usb_gpio_are_rejected(self):
        for key, value in [('board', 'lolin_s2_mini'), ('board', ''), ('sony_pin', 18),
                           ('sony_pin', 9), ('sony_pixels', 17), ('sony_pixels', 0),
                           ('sony_mac', 'not-an-address'), ('rgb_order', 'BGR'),
                           ('sony_name', ''), ('sony_name', 'x\n')]:
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                validate({**SONY, key:value})

    def test_settings_include_only_sony_configuration(self):
        with tempfile.TemporaryDirectory() as directory:
            data = make_settings(validate({**SONY, 'password':'must-not-leak'}), Path(directory)).read_bytes()
            self.assertEqual(stored_byte(data, 'pin'), 4)
            self.assertEqual(stored_byte(data, 'pixels'), 2)
            self.assertEqual(stored_byte(data, 'rgb_order'), 1)
            self.assertIn(b'ILCE-7M4\0', data)
            self.assertNotIn(b'must-not-leak', data)
            self.assertNotIn(b'tally-master', data)

    def test_usb_ports_separate_c3_and_s2(self):
        hardware = [SimpleNamespace(device='S2',description='S2',vid=0x303a,pid=2),
                    SimpleNamespace(device='C3',description='C3',vid=0x303a,pid=0x1001)]
        with patch('desktop.backend.list_ports.comports',return_value=hardware):
            self.assertEqual([p['device'] for p in ports(role='sony-c3')], ['C3'])
            self.assertEqual([p['device'] for p in ports()], ['S2'])
            with self.assertRaises(ValueError):
                FlashJobs().start({**SONY, 'port':'S2'})

    def test_c3_image_offsets_and_chip_never_use_s2_profile(self):
        class Process:
            def __init__(self, *args, **kwargs): self.stdout = io.StringIO('Hash of data verified.\n')
            def __enter__(self): return self
            def __exit__(self, *args): pass
            def kill(self): pass
            def wait(self): return 0
        with patch('desktop.backend.subprocess.Popen',side_effect=Process) as process, patch.object(FlashJobs, '_restart', return_value=False):
            FlashJobs()._flash(validate(SONY), Path('/tmp/chosen-sony-settings.bin'))
            command = process.call_args.args[0]
            self.assertEqual(command[command.index('--chip')+1], 'esp32c3')
            self.assertEqual(command[command.index('--port')+1], 'SONY-USB')
            self.assertIn('0x0', command)
            self.assertNotIn('0x1000', command)
            self.assertEqual(command[command.index('0x9000')+1], '/tmp/chosen-sony-settings.bin')
            self.assertEqual([a for a, _ in images('sony-c3')], ['0x0','0x8000','0xe000','0x10000'])

if __name__ == '__main__': unittest.main()
