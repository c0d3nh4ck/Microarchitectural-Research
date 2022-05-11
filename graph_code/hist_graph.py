# How to Run this python file 
# `python hist_graph.py new_timing_data/time_3 ah`
# Here, argument-1 is path of the folder containing timing values for a particular set of uops
# And, argument-2 is the type of timing value you want to analyze i.e ah, bclf and bh


# Libraries to import
import glob 
import numpy as np
import pandas as pd
import statistics as st
import sys
from matplotlib import pyplot as plt


# Files path being passed down as argument
path1 = sys.argv[1]
path1 += '/time_' + sys.argv[2] + '_*'
txt_files1 = glob.glob(path1)



# Reading lines from files and storing in an array
values1 = []
for filename in txt_files1:
	with open(filename) as f:
		values1 += [int(line.rstrip('\n')) for line in f.readlines()]



# Creating a Dataframe of the values
array1 = np.array(values1)
index_values1 = [i for i in range(len(array1))]
column_values1 = ['values']

df1 = pd.DataFrame(data=array1, index=index_values1, columns=column_values1)



# Removing outliers
Q1 = np.percentile(df1['values'], 25, method='median_unbiased')
Q3 = np.percentile(df1['values'], 75, method='median_unbiased')
IQR = Q3 - Q1

upper = np.where(df1['values'] >= (Q3 + 1.5 * IQR))[0]
lower = np.where(df1['values'] <= (Q3 - 1.5 * IQR))[0]

df1.drop(upper, inplace=True)
df1.drop(lower, inplace=True)



# Mean and Median
print("Mean : ", np.mean(df1['values']))
print("Median : ", np.median(df1['values']))
print("Mode : ", st.mode(df1['values']))



# Plotting a graph
df1.hist(bins=100)

plt.show()
