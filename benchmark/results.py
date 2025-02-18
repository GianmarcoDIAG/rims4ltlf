import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import re

TIMEOUT = 1000.0
MAX_TIRE = 15

results_rims = pd.read_csv(r"rims_h_results.csv")
results_synth = pd.read_csv(r"maxsyft_h_results.csv")

max_k = 0
rims = pd.Series()
synth = pd.Series()
adoption_rims = pd.Series()
adoption_synth = pd.Series()
for i in range(1, MAX_TIRE+1):
    for k, row in results_rims.iterrows():
        if int(re.search(r'\d+', row[2]).group()) == i:
            rims = rims.append(pd.Series([row[-1]]), ignore_index = True)
            adoption_rims = adoption_rims.append(pd.Series([float(row[-2].strip("[]").split("-")[-1])]), ignore_index = True)
            if k > max_k: max_k = k
    for k, row in results_synth.iterrows():
        if int(re.search(r'\d+', row[2]).group()) == i:
            synth = synth.append(pd.Series(row[-1]), ignore_index = True)
            adoption_synth = adoption_synth.append(pd.Series(row[-1]), ignore_index = True)
            if k > max_k: max_k = k
min_k = min(len(rims), len((synth))) - 1

index = pd.Series()
for i in range(1, MAX_TIRE+1):
    index = index.append(pd.Series([i]), ignore_index = True)

while len(rims) < MAX_TIRE:
    rims = rims.append(pd.Series([TIMEOUT]), ignore_index = True)
while len(synth) < MAX_TIRE:
    synth = synth.append(pd.Series([TIMEOUT]), ignore_index = True)
while len(adoption_rims) < MAX_TIRE:
    adoption_rims = adoption_rims.append(pd.Series([TIMEOUT]), ignore_index = True)
while len(adoption_synth) < MAX_TIRE:
    adoption_synth = adoption_synth.append(pd.Series([TIMEOUT]), ignore_index = True)

df = pd.concat([index, synth, rims, adoption_rims], axis=1)
df.name = "RIMS4LTLf vs. MaxSynth"
df.columns = ["Number of Intentions", "MaxSyft", "RIMS4LTLf", "RIMS4LTLf (φ_n)"]
print(df)

df.iloc[:max_k+1].set_index("Number of Intentions").plot(
    kind='bar'
    # color=['orange', 'blue', 'green']
)
plt.yscale("log")
plt.ylim(0.1, 1200)
plt.xticks(rotation = 0) 
plt.axhline(y=1000, color='red', linestyle='--', label='Timeout')  # Horizontal red line
# plt.title(results_df.name)
plt.xlabel("Number of Intentions (n)", fontsize = 13)
plt.ylabel("Time (s)", fontsize=13)
plt.xticks(fontsize = 12)
plt.yticks(fontsize = 12)
plt.legend(fontsize = 13)
plt.savefig('comparison2.pdf', bbox_inches='tight')
plt.show()

# The code below plots two Figures.
# Figure 1 shows the comparison between RIMS4LTLf and MaxSynth wrt all intentions
# Figure 2 shows the comparison between RIMS4LTLf and MaxSynth wrt lowermost priority intention

# results_df = pd.concat([index, rims, synth], axis = 1)
# results_df.name = "RIMS4LTLf vs. MaxSynth"
# results_df.columns = ["Number of Intentions", "RIMS4LTLf", "MaxSynth"]
# print(results_df)

# results_df.iloc[:max_k+1].set_index("Number of Intentions").plot(kind='bar', color=['blue', 'orange'])
# plt.yscale("log")
# plt.ylim(0.1, 1000)
# plt.xticks(rotation = 0) 
# # plt.title(results_df.name)
# plt.xlabel("Number of Intentions (n)", fontsize = 14)
# plt.ylabel("Time (s)", fontsize=14)
# plt.xticks(fontsize = 12)
# plt.yticks(fontsize = 12)
# plt.legend(fontsize = 14)
# plt.savefig('comparison.pdf', bbox_inches='tight')
# plt.show()

# adoption_df = pd.concat([index, adoption_rims, adoption_synth], axis = 1)
# adoption_df.name = "RIMS4LTLf vs. MaxSynth (Single Adoption)"
# adoption_df.columns = ["Lowermost Priority Intention", "RIMS4LTLf", "MaxSynth"]
# print(adoption_df)
# adoption_df.iloc[:max_k+1].set_index("Lowermost Priority Intention").plot(kind='bar', color=['blue', 'orange'])
# plt.yscale("log")
# plt.ylim(0.01, 1000)
# plt.xticks(rotation = 0) 
# # plt.title(results_df.name)
# plt.xlabel("Lowermost Priority Intention")
# plt.ylabel("Time (s)")
# plt.show()
