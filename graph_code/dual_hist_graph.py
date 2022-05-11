# How to Run this python file 
# `python dual_hist_graph.py timing_data/time_17 ah timing_data/time_18 ah`
# Here, argument-1 and 3 is path of the folder containing timing values for a particular set of uops
# And, argument-2 and 4 is the type of timing value you want to analyze i.e ah, bclf and bh


# Libraries to import
import glob 
import numpy as np
import pandas as pd
import sys
import statistics as st
from matplotlib import pyplot as plt


# Files path being passed down as argument
path1 = sys.argv[1]
path1 += '/time_' + sys.argv[2] + '_*'
txt_files1 = glob.glob(path1)

path2 = sys.argv[3]
path2 += '/time_' + sys.argv[4] + '_*'
txt_files2 = glob.glob(path2)



# Reading lines from files and storing in an array
values1 = []
for filename in txt_files1:
    with open(filename) as f:
        values1 += [int(line.rstrip('\n')) for line in f.readlines()]

values2 = []
for filename in txt_files2:
    with open(filename) as f:
        values2 += [int(line.rstrip('\n')) for line in f.readlines()]



# Creating a Dataframe of the values
array1 = np.array(values1)
index_values1 = [i for i in range(len(array1))]
column_values1 = ['values']

array2 = np.array(values2)
index_values2 = [i for i in range(len(array2))]
column_values2 = ['values']

df1 = pd.DataFrame(data=array1, index=index_values1, columns=column_values1)
df2 = pd.DataFrame(data=array2, index=index_values2, columns=column_values2)



# Removing outliers
for df in [df1, df2]:

    Q1 = np.percentile(df['values'], 25, method='median_unbiased')
    Q3 = np.percentile(df['values'], 75, method='median_unbiased')
    IQR = Q3 - Q1

    upper = np.where(df['values'] >= (Q3 + 1.5 * IQR))[0]
    lower = np.where(df['values'] <= (Q3 - 1.5 * IQR))[0]

    df.drop(upper, inplace=True)
    df.drop(lower, inplace=True)



# Mean and Median
for path, df in zip([path1, path2], [df1, df2]):
    print('\n')
    print("====== ", path.split('/')[1], " ======")
    print("Mean : ", np.mean(df['values']))
    print("Median : ", np.median(df['values']))
    # print("Mode : ", st.mode(df['values']))



# Plotting a graph
fig, (ax1, ax2) = plt.subplots(1, 2)


df1.hist(bins=100, ax=ax1)
df2.hist(bins=100, ax=ax2)

ax1.set_title(path1.split('/')[1])
ax2.set_title(path2.split('/')[1])
ax1.set_xlabel('Number of Clock cycles')
ax2.set_xlabel('Number of Clock cycles')
ax1.set_ylabel('Frequency')
ax2.set_ylabel('Frequency')

plt.show()
