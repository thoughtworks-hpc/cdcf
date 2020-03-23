#杨辉三角实例使用说明
##组成
yanghui_triangle_server：组管理服务，只能启动一个<br>
yanghui_triangle_root：杨辉三角主逻辑，负责整个任务的执行，只能启动一个<br>
yanghui_triangle__worker: 计算节点，负责计算，无状态，启动1到N个<br>

##启动参数实例
必须按照顺序启动
```shell
yanghui_triangle_server -H localhost -p 56066
yanghui_triangle_worker -H localhost -p 56067 -g workers@localhost:56066 -r result@localhost:56066 -c compare@localhost:56066 -i 51563
yanghui_triangle_worker -H localhost -p 56067 -g workers@localhost:56066 -r result@localhost:56066 -c compare@localhost:56066 -i 51566
yanghui_triangle_root -H localhost -p 56067 -g workers@localhost:56066 -r result@localhost:56066 -c compare@localhost:56066
```

##输入数据
在yanghui_example.cc文件第11行硬编码
```C++
vector<vector<int> > kYanghuiData = {{5},{7,8},{2,1,4},{4,2,6,1},{2,7,3,4,5},{2,3,7,6,8,3}};
```

##得到结果
启动yanghui_triangle_root 之后，屏幕打印
```shell
compare joined a group:local:compare
yanghui joined a group:local:result
count task finish.
final result:22
```
"final result:22"是结果