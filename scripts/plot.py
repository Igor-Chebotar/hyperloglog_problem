import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')

os.makedirs('plots', exist_ok=True)

# --- График 1: сравнение Nt и F0 для каждой конфигурации ---
for cfg in [1, 2, 3]:
    fname = f'data/single_run_cfg{cfg}.csv'
    if not os.path.exists(fname):
        continue
    df = pd.read_csv(fname)

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(df['percent'], df['exact'], 'b-o', label='Точное F₀', markersize=4)
    ax.plot(df['percent'], df['estimate_hll'], 'r--s',
            label='Оценка HLL', markersize=4)
    ax.plot(df['percent'], df['estimate_improved'], 'g--^',
            label='Оценка HLL++ (improved)', markersize=4)
    ax.set_xlabel('Обработано потока (%)')
    ax.set_ylabel('Количество уникальных элементов')
    ax.set_title(f'Конфигурация {cfg}: сравнение оценки и точного значения')
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(f'plots/comparison_cfg{cfg}.png', dpi=150)
    plt.close(fig)
    print(f'Saved plots/comparison_cfg{cfg}.png')

# --- График 2: E(Nt) +- sigma для каждой конфигурации ---
for cfg in [1, 2, 3]:
    fname = f'data/aggregated_cfg{cfg}.csv'
    if not os.path.exists(fname):
        continue
    df = pd.read_csv(fname)

    fig, axes = plt.subplots(1, 2, figsize=(16, 6))

    # Стандартный HLL
    ax = axes[0]
    ax.plot(df['percent'], df['mean_exact'], 'b-', label='Точное F₀', linewidth=2)
    ax.plot(df['percent'], df['mean_hll'], 'r--', label='E(Nt) HLL', linewidth=2)
    ax.fill_between(df['percent'],
                    df['mean_hll'] - df['std_hll'],
                    df['mean_hll'] + df['std_hll'],
                    alpha=0.25, color='red', label='±σ HLL')
    ax.set_xlabel('Обработано потока (%)')
    ax.set_ylabel('Количество уникальных элементов')
    ax.set_title(f'Конфигурация {cfg}: стандартный HyperLogLog')
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Улучшенный
    ax = axes[1]
    ax.plot(df['percent'], df['mean_exact'], 'b-', label='Точное F₀', linewidth=2)
    ax.plot(df['percent'], df['mean_improved'], 'g--',
            label='E(Nt) HLL++', linewidth=2)
    ax.fill_between(df['percent'],
                    df['mean_improved'] - df['std_improved'],
                    df['mean_improved'] + df['std_improved'],
                    alpha=0.25, color='green', label='±σ HLL++')
    ax.set_xlabel('Обработано потока (%)')
    ax.set_ylabel('Количество уникальных элементов')
    ax.set_title(f'Конфигурация {cfg}: улучшенный HyperLogLog++')
    ax.legend()
    ax.grid(True, alpha=0.3)

    fig.tight_layout()
    fig.savefig(f'plots/stats_cfg{cfg}.png', dpi=150)
    plt.close(fig)
    print(f'Saved plots/stats_cfg{cfg}.png')

# --- График 3: зависимость ошибки от B ---
fname = 'data/b_test.csv'
if os.path.exists(fname):
    df = pd.read_csv(fname)
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.bar(df['b'], df['error_pct'], color='steelblue', alpha=0.8)
    ax.set_xlabel('Параметр B (число бит индекса)')
    ax.set_ylabel('Относительная ошибка (%)')
    ax.set_title('Зависимость ошибки HyperLogLog от параметра B')
    ax.set_xticks(df['b'])
    ax.grid(True, alpha=0.3, axis='y')
    fig.tight_layout()
    fig.savefig('plots/b_test.png', dpi=150)
    plt.close(fig)
    print('Saved plots/b_test.png')

# --- График 4: сравнение памяти ---
fname = 'data/memory.csv'
if os.path.exists(fname):
    df = pd.read_csv(fname)
    fig, ax = plt.subplots(figsize=(8, 5))
    bars = ax.bar(df['version'], df['bytes'], color=['#e74c3c', '#2ecc71'],
                  alpha=0.85)
    for bar, val in zip(bars, df['bytes']):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 5,
                f'{val} bytes', ha='center', va='bottom', fontsize=12)
    ax.set_ylabel('Размер регистров (байт)')
    ax.set_title('Сравнение потребления памяти')
    ax.grid(True, alpha=0.3, axis='y')
    fig.tight_layout()
    fig.savefig('plots/memory.png', dpi=150)
    plt.close(fig)
    print('Saved plots/memory.png')

print('\nAll plots generated!')
