"""Guard the checked-in DOS inventory and fail closed on missing/stale evidence."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('transition_audit', ROOT/'scripts/audit_transitions.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class TransitionAuditTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(audit.MANIFEST.read_text())

    def test_checked_in_inventory_and_document(self):
        audit.validate(self.data)
        self.assertEqual(audit.DOCUMENT.read_text(), audit.render(self.data))

    def test_missing_invalid_and_stale_inventory_files(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory)/'inventory.json'
            document = Path(directory)/'inventory.md'
            with patch.object(audit, 'MANIFEST', manifest), patch.object(audit, 'DOCUMENT', document), patch.object(sys, 'argv', ['audit', '--check']):
                with self.assertRaises(SystemExit) as missing_manifest:
                    audit.main()
                self.assertEqual(missing_manifest.exception.code, 1)
                manifest.write_text('{bad json')
                with self.assertRaises(SystemExit):
                    audit.main()
                manifest.write_text(json.dumps(self.data))
                with self.assertRaises(SystemExit):
                    audit.main()
                document.write_text('stale')
                with self.assertRaises(SystemExit):
                    audit.main()
                with patch.object(sys, 'argv', ['audit', '--write']):
                    audit.main()
                audit.main()

    def test_rejects_semantic_and_reference_corruption(self):
        def mutate(field, value):
            data = copy.deepcopy(self.data)
            data['transitions'][0][field] = value
            return data
        cases = [mutate('palette', ''), mutate('evidence', []),
                 mutate('implementation', []), mutate('source', ['unknown']),
                 mutate('from', 'unmapped'), mutate('status', 'pixel-perfect'),
                 mutate('gaps', [])]
        data = copy.deepcopy(self.data)
        data['transitions'] = [t for t in data['transitions'] if t['id'] != 'psyched-progress']
        cases.append(data)
        for key, value in [('native_ui', {}), ('dos_exits', {}), ('original_sha256', {}),
                           ('journeys', {'bad': ['startup', 'quick-resume']})]:
            data = copy.deepcopy(self.data)
            data[key] = value
            cases.append(data)
        data = copy.deepcopy(self.data)
        next(iter(data['references'].values()))['anchor'] = 'missing original function'
        cases.append(data)
        data = copy.deepcopy(self.data)
        next(iter(data['references'].values()))['path'] = '../outside'
        cases.append(data)
        data = copy.deepcopy(self.data)
        key = next(iter(data['original_sha256']))
        data['original_sha256'][key] = '0'*64
        cases.append(data)
        data = copy.deepcopy(self.data)
        data['transitions'].append(data['transitions'][0])
        cases.append(data)
        for index, data in enumerate(cases):
            with self.subTest(case=index), self.assertRaises(ValueError):
                audit.validate(data)

    def test_checks_survive_python_optimization(self):
        # Python -O removed the previous assert-based validation entirely.
        program = "import runpy; a=runpy.run_path('scripts/audit_transitions.py'); a['validate']({'schema': 999})"
        result = subprocess.run([sys.executable, '-O', '-c', program], cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('ValueError', result.stderr)
