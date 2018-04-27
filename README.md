# COFLOW IN HRRN

## 介绍
本项目为测试验证coflow特性为目的开发。

## 编译
###windows

windows下使用vs2015。
在GUI界面依次生成simple_uv,dfs_master,dfs_slave,dfs_client项目

### linux（尚未完成）：  

编译libuv需要：libtool，autoconf

编译本项目需要：cmake


##测试
###windows
执行bin文件夹下的runtest.Win.bat，能够启动一个master节点和3个slave节点（可修改bat文件调整服务器个数）。接着执行dfs_client.exe程序，即可验证coflow调度的影响。