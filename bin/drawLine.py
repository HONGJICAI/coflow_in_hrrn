import matplotlib.pyplot as plt
import numpy as np

#添加图形属性
plt.xlabel('coflow ID')
plt.ylabel('CCT')
plt.title('The statistics of experiment II')
a = plt.subplot(1, 1, 1)

plt.ylim=(10, 90000)
x0 = [1, 2, 3, 4, 5, 6, 7,8]
x1 = [0.7, 1.7, 2.7, 3.7, 4.7, 5.7, 6.7,7.7]
scf=[82845,8231,3450,101324,81567,5270,102671,27140]
chrrn=[4075,5480,6787,9616,12113,12395,17730,22554]


#这里需要注意在画图的时候加上label在配合plt.legend（）函数就能直接得到图例，简单又方便！

plt.bar(x0, scf, facecolor='black', width=0.3, label = 'SCF')
plt.bar(x1, chrrn, facecolor='gray', width=0.3, label = 'CHRRN')

plt.legend()

plt.show()