#!/usr/bin/env python3
"""Validate the source/evidence inventory and render its review document.

This checks inventory structure and reference/enum coverage, not game behaviour.
No game assets or external dependencies are required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'docs/dos_state_transitions.json'
DOCUMENT = ROOT / 'docs/DOS_STATE_TRANSITIONS.md'
# Semantic route coverage is independent of the input manifest: deleting both
# a transition and its coverage mapping must not silently pass validation.
REQUIRED_TRANSITIONS = {
    'startup', 'signon-ack', 'rating-title', 'title-credits', 'credits-scores',
    'scores-demo', 'demo-title', 'demo-menu', 'menu-episode',
    'episode-difficulty', 'episode-replace', 'replace-yes', 'replace-no',
    'difficulty-setup', 'setup-psyched', 'psyched-progress', 'psyched-reveal',
    'reveal-game', 'game-pause', 'pause-game', 'game-menu', 'menu-game',
    'save-panel', 'load-panel', 'save-work', 'load-work', 'ordinary-load-setup',
    'quicksave', 'quickload-confirm', 'quickload-yes', 'quickload-no',
    'quick-resume', 'storage-error', 'death-turn', 'death-fizzle', 'death-ack',
    'death-restart', 'restart-reveal', 'death-final', 'exit-completion',
    'next-setup', 'secret-completion', 'secret-entry', 'secret-return',
    'boss-victory', 'victory-stats', 'victory-article', 'article-scores',
    'qualify', 'not-qualified', 'name-save', 'results-title', 'quit-menu',
    'quit-game', 'quit-no', 'quit-yes', 'quit-no-game', 'end-game', 'end-yes',
    'end-no', 'menu-scores', 'scores-menu', 'menu-attract', 'preload-error',
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def reference(ref):
    require(isinstance(ref['path'], str), 'Reference path must be text')
    path = (ROOT / ref['path']).resolve()
    require(path.is_relative_to(ROOT.resolve()) and not Path(ref['path']).is_absolute(),
            f"Reference escapes repository: {ref['path']}")
    require(isinstance(ref['anchor'], str) and ref['anchor'].strip(), 'Empty reference anchor')
    text = path.read_text(encoding='latin1')
    require(ref['anchor'] in text, f'Missing anchor: {ref}')
    line = text[:text.index(ref['anchor'])].count('\n') + 1
    return f"[{ref.get('label', ref['anchor']).strip()}](../{ref['path']}#L{line})"


def validate(data):
    require(data['schema'] == 1, 'Invalid inventory structure')
    require(isinstance(data['transitions'], list), 'Transitions must be a list')
    require(data['states'] and len(data['states']) == len(set(data['states'])), 'Empty/duplicate states')
    ids = [t['id'] for t in data['transitions']]
    require(REQUIRED_TRANSITIONS <= set(ids), f'Missing required routes: {REQUIRED_TRANSITIONS - set(ids)}')
    require(len(ids) == len(set(ids)), 'Duplicate transition IDs')
    states = set(data['states'])
    refs = data['references']
    evidence = data['evidence']
    gaps = data['gaps']
    for ref in refs.values():
        reference(ref)
    original_refs = {r['path'] for r in refs.values() if r['path'].startswith('WOLFSRC/')}
    require(original_refs <= data['original_sha256'].keys(), 'Unhashed original source reference')
    for name, digest in data['original_sha256'].items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest, f'Re-audit changed source: {name}')
    for item in evidence.values():
        require(item['scope'] and item['observation'] and item['limits'], 'Invalid inventory structure')
        for ref in item['refs']:
            reference(ref)
    for transition in data['transitions']:
        for field in ('group', 'from', 'to', 'trigger', 'source', 'timing', 'palette', 'artwork', 'sound', 'input', 'implementation', 'evidence', 'status', 'gaps'):
            require(field in transition, f"{transition['id']}: missing {field}")
        require(transition['from'] in states and transition['to'] in states, 'Invalid inventory structure')
        require(transition['implementation'] and transition['evidence'], f"{transition['id']}: no implementation/evidence")
        require(transition['source'], f"{transition['id']}: no original source")
        require(set(transition['source'] + transition['implementation']) <= refs.keys(), 'Invalid inventory structure')
        require(set(transition['evidence']) <= evidence.keys(), 'Invalid inventory structure')
        require(set(transition['gaps']) <= gaps.keys(), 'Invalid inventory structure')
        require(transition['status'] in data['statuses'], 'Invalid inventory structure')
        for field in ('group', 'trigger', 'timing', 'palette', 'artwork', 'sound', 'input'):
            require(transition[field].strip(), f"{transition['id']}: empty {field}")
        if transition['status'] == 'partial':
            require(transition['gaps'], f"{transition['id']}: partial without a tracked gap")
    ui = set(re.findall(r'\bUI_[A-Z_]+\b', (ROOT / 'src/engine/WL_MAIN.h').read_text()))
    require(set(data['native_ui']) == ui, f"UI coverage differs: {set(data['native_ui']) ^ ui}")
    exits_text = (ROOT / 'WOLFSRC/WL_DEF.H').read_text(encoding='latin1')
    exit_enum = re.search(r'typedef\s+enum\s*\{\s*(ex_stillplaying[\s\S]*?)\}\s*exit_t', exits_text)
    require(exit_enum, 'Cannot locate DOS exit_t')
    exits = set(re.findall(r'\bex_\w+\b', exit_enum[1]))
    require(set(data['dos_exits']) == exits, f"DOS exit coverage differs: {set(data['dos_exits']) ^ exits}")
    for mapping in (data['native_ui'], data['dos_exits']):
        for entry in mapping.values():
            require(entry['transitions'] or entry['classification'] == 'declaration-only', 'Unmapped active state')
            require(entry['classification'] and entry['note'], 'Invalid inventory structure')
            require(set(entry['transitions']) <= set(ids), 'Invalid inventory structure')
    for gap in gaps.values():
        require(gap['priority'] and gap['finding'] and gap['close_with'], 'Invalid inventory structure')
        require(set(gap['refs']) <= refs.keys(), 'Invalid inventory structure')
    edges = {t['id']: t for t in data['transitions']}
    for journey in data['journeys'].values():
        require(journey, 'Empty journey')
        previous = None
        for step in journey:
            edge = edges[step]
            require(previous is None or previous == edge['from'], f'Disconnected journey at {step}')
            previous = edge['to']
    used_states = {t[k] for t in data['transitions'] for k in ('from', 'to')}
    require(used_states == states, f'Unmapped phases: {states - used_states}')


def render(data):
    def links(names):
        return '; '.join(reference(data['references'][name]) for name in names) or 'No DOS-equivalent implementation.'

    lines = ['# DOS interaction and state-transition inventory', '', *data['introduction'], '',
             '## How to read the evidence', '']
    for name, meaning in data['statuses'].items():
        lines.append(f'- **{name}**: {meaning}')
    lines += ['', f"Inventory: **{len(data['states'])} phases, {len(data['transitions'])} directed transition records**, "
              f"all **{len(data['native_ui'])} native UI states** and **{len(data['dos_exits'])} DOS exit values** accounted for.", '',
              'A linked test is evidence only for its stated assertions. It does not prove the complete transition, '
              'DOS fade/audio equivalence, or a continuous playthrough. This audit adds no new runtime pass claims.', '',
              '## Principal journeys', '', '```mermaid', 'flowchart LR',
              '  Menu --> Episode --> Difficulty --> Setup --> Psyched --> Reveal --> Gameplay',
              '  Gameplay --> DeathTurn --> RedFizzle --> DeathAck',
              '  DeathAck --> Restart --> Reveal',
              '  DeathAck --> Scores --> Title',
              '  Gameplay --> Completion --> Psyched',
              '  Gameplay --> SecretCompletion --> Psyched',
              '  Gameplay --> BJOrBossReplay --> Victory --> Article --> Scores',
              '  Menu --> OrdinaryLoad --> Setup',
              '  Gameplay --> QuickLoad --> Gameplay',
              '  Menu --> QuitConfirm --> Exit',
              '```', '',
              'The graph describes DOS routes. In particular, death restart bypasses Get Psyched, '
              'and established-slot quickload returns to the existing play loop. Differences below remain open.', '']
    by_id = {t['id']: t for t in data['transitions']}
    for name, route in data['journeys'].items():
        description = ' → '.join([by_id[route[0]]['from']] + [by_id[step]['to'] for step in route])
        lines.append(f'- **{name}**: {description}. Rows: {", ".join(route)}.')
    lines += ['', '## Findings that prevent a DOS-equivalence claim', '']
    for name, gap in data['gaps'].items():
        affected = ', '.join(t['id'] for t in data['transitions'] if name in t['gaps'])
        lines += [f"### {name} — {gap['priority']}: {gap['title']}", '', gap['finding'], '',
                  f"Source/implementation: {links(gap['refs'])}", '', f'Rows: {affected}.', '',
                  f"Close with: {gap['close_with']}", '']
    lines += ['## Transition records', '']
    group = None
    for t in data['transitions']:
        if t['group'] != group:
            group = t['group']
            lines += [f'### {group}', '']
        ev = ', '.join(f'[{name}](#{name.lower()})' for name in t['evidence']) or '**No direct executed regression**'
        lines += [f"#### {t['id']}: {t['from']} → {t['to']}", '',
                  f"**Trigger:** {t['trigger']} **Status:** `{t['status']}`.", '',
                  f"- **DOS source:** {links(t['source'])}",
                  f"- **Timing:** {t['timing']}",
                  f"- **Palette:** {t['palette']}",
                  f"- **Artwork/layout:** {t['artwork']}",
                  f"- **Sound/music:** {t['sound']}",
                  f"- **Input/state:** {t['input']}",
                  f"- **Revive:** {links(t['implementation'])}",
                  f"- **Regression evidence:** {ev}. Open findings: {', '.join(t['gaps']) or 'none recorded for this row; evidence limits still apply'}.", '']
    lines += ['## Evidence register', '']
    for name, ev in data['evidence'].items():
        lines += [f'### {name}', '', f"**Scope:** {ev['scope']}", '',
                  '; '.join(reference(ref) for ref in ev['refs']), '',
                  f"**Observed:** {ev['observation']}", '', f"**Does not establish:** {ev['limits']}", '']
    for label, mapping in [('Native UI coverage', data['native_ui']), ('DOS playstate coverage', data['dos_exits'])]:
        lines += [f'## {label}', '', '| State | Classification | Rows | Boundary |', '| --- | --- | --- | --- |']
        for name, entry in mapping.items():
            lines.append(f"| `{name}` | {entry['classification']} | {', '.join(entry['transitions']) or 'declaration only'} | {entry['note']} |")
        lines.append('')
    lines += ['## Excluded editions, hardware and native adaptations', '', *data['boundaries'], '',
              '## Keeping this inventory current', '',
              'The JSON manifest is authoritative for this document. After reviewing changed source/behaviour, '
              'update the records and evidence, then run:', '', '```sh',
              'python3 scripts/audit_transitions.py --write', 'python3 scripts/audit_transitions.py --check', '```', '',
              'The check rejects missing fields, stale source hashes/anchors, unknown evidence/gaps, disconnected '
              'journeys, unclassified native UI states, and unclassified DOS exit values. It also checks that this '
              'document matches the manifest. It does **not** execute the game, discover arbitrary new branch '
              'conditions, or prove that a cited test asserts every property in its row.', '',
              'For a behaviour change, add an edge-specific regression before changing its evidence status. '
              'Record the initial/intermediate/final framebuffer, palette, time, sound events, input release/ack, '
              'map/lives/save/RNG and SDL window/logical geometry where applicable. A successful endpoint or '
              'a helper that drains loading is insufficient evidence for the presentation between endpoints.', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--write', action='store_true')
    group.add_argument('--check', action='store_true')
    args = parser.parse_args()
    try:
        data = json.loads(MANIFEST.read_text(encoding='utf-8'))
        validate(data)
        expected = render(data)
        if args.write:
            DOCUMENT.write_text(expected, encoding='utf-8')
        else:
            require(DOCUMENT.read_text(encoding='utf-8') == expected, 'Document stale; run --write after review')
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        parser.exit(1, f'Audit failed: {error}\n')
    print(f"Validated {len(data['transitions'])} transitions, {len(data['states'])} phases, "
          f"{len(data['native_ui'])} UI states, {len(data['dos_exits'])} DOS exits; "
          f"{len(data['gaps'])} findings remain open. No runtime behaviour was certified by this check.")


if __name__ == '__main__':
    main()
