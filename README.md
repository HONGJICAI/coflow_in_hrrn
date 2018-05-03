# COFLOW IN HRRN

## 介绍
本项目为测试验证coflow特性为目的开发。

## 编译

### windows

windows下使用vs2015。
在GUI界面依次生成simple_uv,dfs_master,dfs_slave,dfs_client项目

### linux（尚未完成）：  

编译libuv需要：libtool，autoconf

编译本项目需要：cmake


## 测试
### windows
运行dfs_master.exe需要携带两个参数：slaveNum和clientNum。
运行dfs_slave.exe需要携带一个参数：slave的serverSocket的端口。

使用脚本一键测试：执行bin文件夹下的runtest.Win.bat，能够启动一个master节点，3个slave节点，5个client节点（可修改bat文件进行调整）。统计结果将会输出在result.txt中。