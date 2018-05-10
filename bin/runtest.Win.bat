%coflowNum,slaveNum自行设置%
set coflowNum=8
set slaveNum=4

start /d "." dfs_master.exe %slaveNum% %coflowNum%
%若要修改slave个数，仿照下述例子自行添加。携带参数为端口号%
start /d "." dfs_slave.exe 100
start /d "." dfs_slave.exe 200
start /d "." dfs_slave.exe 300
start /d "." dfs_slave.exe 400

set i=0
:createCoflow
set /a i+=1
start /d "." dfs_client.exe
if %i% NEQ %coflowNum% goto createCoflow
