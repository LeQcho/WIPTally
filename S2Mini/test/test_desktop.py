import json
import io
from pathlib import Path
import struct
import sys
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from urllib.request import Request, urlopen
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from desktop.backend import FlashJobs, make_settings, settings_rows, validate
from desktop.server import create_server
from desktop.usb_flash import POWER_CYCLE, restart_message, write_verified

MASTER = {"role":"master", "ssid":"Réseau, test", "password":"mot-de-passe", "atem":"192.0.2.10", "inputs":8, "port":"SIMULATION"}

def stored_byte(binary, key):
    # NVS entries: key at byte 8 and scalar value at byte 24.
    index = binary.index(key.encode() + b'\0') - 8
    assert binary[index+1] == 1  # U8
    return binary[index+24]

class SettingsTests(unittest.TestCase):
    def test_real_flash_command_contains_selected_configuration_not_personal_master_settings(self):
        class Process:
            def __init__(self, command, **kwargs):
                self.command = command
                self.stdout = io.StringIO('Hash of data verified.\n')
            def __enter__(self): return self
            def __exit__(self, *args): pass
            def kill(self): pass
            def wait(self): return 0
        with tempfile.TemporaryDirectory() as tmp, patch('desktop.usb_flash.subprocess.Popen',side_effect=Process) as process, patch('desktop.backend.restart_message',return_value=POWER_CYCLE):
            settings=Path(tmp)/'chosen.bin';settings.write_bytes(b'x'*0x5000)
            jobs=FlashJobs()
            jobs._flash({'role':'receiver-rgb','port':'SELECTED-USB'},settings)
            command=process.call_args.args[0]
            self.assertEqual(command[command.index('--port')+1],'SELECTED-USB')
            self.assertEqual(command[command.index('0x9000')+1],str(settings))
            self.assertTrue(any('receiver-rgb' in value for value in command))
            self.assertFalse(any('network-settings.bin' in value for value in command))
            self.assertEqual(command[command.index('--after')+1], 'watchdog_reset')
            self.assertEqual(process.call_count, 1)

    def test_camera_preconfiguration_and_master_input_count(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = validate({"role":"receiver-rgb", "camera":6})
            binary = make_settings(config, Path(tmp)).read_bytes()
            self.assertEqual(len(binary), 0x5000)
            self.assertEqual(stored_byte(binary, "camera"), 6)
            self.assertEqual(stored_byte(binary, "rgb_order"), 1)
            self.assertEqual(stored_byte(binary, "rgb_level"), 130)
            self.assertNotIn(b"mot-de-passe", binary)
            binary = make_settings(validate(MASTER), Path(tmp)).read_bytes()
            self.assertEqual(stored_byte(binary, "inputs"), 8)
            self.assertEqual(stored_byte(binary, "rgb_level"), 130)
            self.assertEqual(stored_byte(binary, "demo"), 0)
            self.assertIn("Réseau, test".encode()+b'\0', binary)
            self.assertIn(b"mot-de-passe\0", binary)

    def test_invalid_settings_never_start_flash(self):
        for field, value in [("inputs",0),("inputs",41),("inputs",True),("inputs","1; echo x"),
                             ("atem","255.255.255.255"),("atem","224.0.0.1"),
                             ("password","short"),("ssid","é"*17)]:
            with self.subTest(field=field,value=value), self.assertRaises(ValueError):
                validate({**MASTER,field:value})
        for role in ("receiver", "receiver-rgb"):
            self.assertEqual(validate({"role":role,"camera":40})["camera"],40)
        with self.assertRaises(ValueError): validate({**MASTER,"password":""})
        self.assertEqual(validate({**MASTER,"password":"","open_wifi":True})["password"],"")
        self.assertEqual(validate({"role":"receiver-rgb","camera":2,"rgb_order":"GRB"})['rgb_order'],'GRB')
        with self.assertRaises(ValueError): validate({"role":"receiver-rgb","camera":2,"rgb_order":"BGR"})

    def test_rgb_brightness_reaches_flash_settings(self):
        for percentage, level in (("25", 64), ("50", 130), ("75", 191), ("100", 255)):
            with self.subTest(percentage=percentage), tempfile.TemporaryDirectory() as tmp:
                config = validate({"role": "receiver-rgb", "rgb_brightness": percentage})
                settings = make_settings(config, Path(tmp)).read_bytes()
                self.assertEqual(stored_byte(settings, "rgb_level"), level)
        for invalid in (0, 60, 101, True, None):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                validate({"role": "receiver-rgb", "rgb_brightness": invalid})
        classic = validate({"role": "receiver", "rgb_brightness": "100"})
        self.assertNotIn("rgb_level", classic)

    def test_master_brightness_is_written_to_the_master_namespace(self):
        for percentage, level in (("25", 64), ("50", 130), ("75", 191), ("100", 255)):
            with self.subTest(percentage=percentage), tempfile.TemporaryDirectory() as tmp:
                config = validate({**MASTER, "brightness": percentage})
                rows = settings_rows(config)
                self.assertEqual(rows[0], ("tally-master", "namespace", "", ""))
                self.assertIn(("rgb_level", "data", "u8", level), rows)
                self.assertEqual(stored_byte(make_settings(config, Path(tmp)).read_bytes(), "rgb_level"), level)
        for invalid in (0, 60, 101, True, None):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                validate({**MASTER, "brightness": invalid})

    def test_independent_oled_durations_reach_master_nvs(self):
        durations = {"oled_atem": 5, "oled_wifi": 10, "oled_led": 15, "oled_rx": 30}
        with tempfile.TemporaryDirectory() as tmp:
            config = validate({**MASTER, **durations})
            binary = make_settings(config, Path(tmp)).read_bytes()
            for key, seconds in durations.items():
                self.assertEqual(stored_byte(binary, key), seconds)
            default_binary = make_settings(validate(MASTER), Path(tmp)).read_bytes()
            for key in durations:
                self.assertEqual(stored_byte(default_binary, key), 5)
        for key in durations:
            for valid in (1, 120, "10"):
                self.assertEqual(validate({**MASTER, key: valid})[key], int(valid))
            for invalid in (0, 121, -1, True, None, "", 1.5, "5.0", "1; echo x"):
                with self.subTest(key=key, invalid=invalid), self.assertRaises(ValueError):
                    validate({**MASTER, key: invalid})
        self.assertNotIn("oled_atem", validate({"role": "receiver-rgb", "camera": 1}))

    def test_restart_follows_the_same_usb_device_after_renumbering(self):
        original = SimpleNamespace(device='COM5', location='1-2:1.0')
        other = SimpleNamespace(device='COM5', vid=0x303a, pid=0x80c2, location='1-3:1.0')
        target = SimpleNamespace(device='COM7', vid=0x303a, pid=0x80c2, location='1-2:1.1')
        logs = []
        with patch('desktop.usb_flash.list_ports.comports', side_effect=[[other], [other, target]]), patch('desktop.usb_flash.time.sleep'):
            result = restart_message(original.device, original, 'esp32s2', 0, logs.append)
        self.assertIn('redémarrée automatiquement', result)
        self.assertIn('COM7', logs[0])

    def test_restart_fallback_uses_power_cycle_when_only_bootloader_returns(self):
        boot = SimpleNamespace(device='USB', vid=0x303a, pid=2)
        with patch('desktop.usb_flash.list_ports.comports', return_value=[boot]), patch('desktop.usb_flash.time.sleep'):
            self.assertEqual(restart_message('USB', boot, 'esp32s2', 0, lambda _: None), POWER_CYCLE)

    def test_reset_error_does_not_turn_verified_images_into_a_flash_failure(self):
        hashes = ['Hash of data verified.'] * 5
        reset = 'Hard resetting with a watchdog...'
        self.assertTrue(write_verified(2, hashes + [reset], 5))
        self.assertFalse(write_verified(2, hashes[:-1] + [reset], 5))
        self.assertFalse(write_verified(2, hashes, 5))

    def test_single_flash_and_cleanup(self):
        jobs = FlashJobs(simulate=True)
        captured = []
        from desktop import backend
        original = backend.make_settings
        def capture(config, directory):
            captured.append(directory)
            return original(config, directory)
        with patch('desktop.backend.make_settings',side_effect=capture), patch.object(jobs, '_flash') as physical:
            jobs.start(MASTER)
            with self.assertRaises(ValueError): jobs.start(MASTER)
            deadline = time.monotonic()+10
            while jobs.snapshot()['status']=='running' and time.monotonic()<deadline: time.sleep(.02)
            self.assertEqual(jobs.snapshot()['status'],'success')
            self.assertFalse(physical.called)
        self.assertTrue(captured)
        self.assertTrue(all(not p.exists() for p in captured))
        self.assertNotIn(MASTER['password'],json.dumps(jobs.snapshot()))


class HttpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server=create_server(simulate=True)
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.base=f'http://127.0.0.1:{cls.server.server_port}'

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown();cls.server.server_close();cls.thread.join()

    def test_local_api_requires_token_and_rejects_other_origins(self):
        state=json.load(urlopen(self.base+'/api/state'))
        self.assertTrue(state['simulate'])
        for headers in ({}, {'X-Tally-Token':state['token'],'Origin':'https://example.com'}):
            with self.assertRaises(HTTPError) as error:
                urlopen(Request(self.base+'/api/flash',data=json.dumps(MASTER).encode(),headers=headers))
            self.assertEqual(error.exception.code,403)
        with self.assertRaises(HTTPError) as error:
            urlopen(Request(self.base+'/api/state',headers={'Host':'example.com'}))
        self.assertEqual(error.exception.code,403)
        req=Request(self.base+'/api/check',data=json.dumps(MASTER).encode(),headers={'X-Tally-Token':state['token'],'Content-Type':'application/json'})
        self.assertEqual(urlopen(req).status,202)
        deadline=time.monotonic()+10
        while self.server.jobs.snapshot()['status']=='running' and time.monotonic()<deadline:time.sleep(.02)
        self.assertEqual(self.server.jobs.snapshot()['status'],'success')

if __name__ == '__main__': unittest.main()
