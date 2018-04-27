./dfs_master.exe &
sleep 1
./dfs_slave.exe 100 &
./dfs_slave.exe 200 &
./dfs_slave.exe 300 &
#sleep 1
#./dfs_client.exe
sleep 1
pkill dfs_master.exe
pkill dfs_slave.exe