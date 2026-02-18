#!/usr/bin/env python3
import os
import csv
import sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
RESULTS = os.path.join(ROOT, 'perf_results')
SUMMARY = os.path.join(RESULTS, 'summary.csv')
AGG_OUT = os.path.join(RESULTS, 'aggregate_summary.csv')

if not os.path.exists(SUMMARY):
    print('No summary.csv found at', SUMMARY)
    sys.exit(1)

# Read summary rows
rows = []
with open(SUMMARY, newline='') as f:
    r = csv.DictReader(f)
    for rec in r:
        rec['actors'] = int(rec['actors'])
        rec['messages'] = int(rec['messages'])
        rec['run'] = int(rec['run'])
        rec['exit_code'] = int(rec['exit_code'])
        rec['total_expected'] = int(rec['total_expected'])
        rec['received'] = int(rec['received'])
        rec['elapsed_s'] = float(rec['elapsed_s'])
        rec['max_cpu_pct'] = float(rec['max_cpu_pct'])
        rec['max_rss_kb'] = float(rec['max_rss_kb'])
        rows.append(rec)

# Group by (actors,messages)
groups = defaultdict(list)
for r in rows:
    key = (r['actors'], r['messages'])
    groups[key].append(r)

# Aggregate
agg_rows = []
for k, lst in sorted(groups.items()):
    actors, messages = k
    runs = len(lst)
    ok_runs = sum(1 for x in lst if x['exit_code'] == 0)
    avg_elapsed = sum(x['elapsed_s'] for x in lst) / runs
    avg_received = sum(x['received'] for x in lst) / runs
    avg_cpu = sum(x['max_cpu_pct'] for x in lst) / runs
    avg_rss = sum(x['max_rss_kb'] for x in lst) / runs
    agg_rows.append({
        'actors': actors,
        'messages': messages,
        'runs': runs,
        'ok_runs': ok_runs,
        'avg_elapsed_s': f"{avg_elapsed:.3f}",
        'avg_received': f"{avg_received:.1f}",
        'avg_max_cpu_pct': f"{avg_cpu:.2f}",
        'avg_max_rss_kb': f"{avg_rss:.1f}",
    })

# Write aggregate CSV
with open(AGG_OUT, 'w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=list(agg_rows[0].keys()))
    writer.writeheader()
    for r in agg_rows:
        writer.writerow(r)

print('Wrote aggregate summary to', AGG_OUT)

# Try to produce simple plots if matplotlib available
try:
    import pandas as pd
    import matplotlib.pyplot as plt
    df = pd.DataFrame(agg_rows)
    # plot elapsed vs actors for each messages
    for m in sorted(df['messages'].unique()):
        dff = df[df['messages'] == m].sort_values('actors')
        plt.plot(dff['actors'], dff['avg_elapsed_s'].astype(float), marker='o', label=f'm={m}')
    plt.xlabel('actors')
    plt.ylabel('avg elapsed (s)')
    plt.title('Elapsed vs actors')
    plt.legend()
    png = os.path.join(RESULTS, 'elapsed_vs_actors.png')
    plt.savefig(png)
    print('Saved plot', png)
except Exception as e:
    print('Skipping plots (matplotlib/pandas not available):', e)
