# asoul_bbspace

## 配置方法

要求python3.8及以上

以下方法二选一：

1. networkx稳定版

`pip install networkx`
  
注意：此版本(2.7)与文档版本(3.0)存在差别，部分算法接口可能不同

2. networkx开发版

首先卸载已安装的networkx

  `pip uninstall networkx`
```
  git clone https://github.com/networkx/networkx.git
  cd networkx
  pip install -e .[default]
  ```
