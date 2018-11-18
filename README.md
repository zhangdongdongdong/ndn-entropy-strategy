# ndn-entropy-strategy
Forwarding strategy based on entropy weighting method  in NDN

1、The experiment is based on ndnSIM 1.0, if you want to run the experiment, please install ndnSIM 1.0 first.
2、Copy the given code to the directory where ndnSIM is already installed.
3、Change all referenced paths in the code "/home/duoduo" to the user's own home directory.
4、Run the myrunzoomroute.py file to run the experiment

Directory structure description:
（1）The experimentResult folder is used to store the resulting data. If the original ndnSIM directory does not have the folder, it needs to be recreated.
（2）Topologies folder: inside is the network topology diagram used in the experiment
（3）Ns-3 folder: If ndnSIM is installed, this ns-3 folder is already there. We only need to copy the files in the ns-3 folder. Note that some of the folders under the ns-3 folder may already exist. At this time, you only need to copy the files in the folder.
（4）The ns-3/src/ndnSIM/model/fw folder corresponds to our forwarding strategy based on entropy weighting.


1、该实验是基于ndnSIM 1.0，如果要运行实验请先安装ndnSIM 1.0
2、将给出的代码拷贝到已经安装好的ndnSIM的目录下
3、将代码中所有引用的路径“/home/duoduo”改成本机自己的用户主目录
4、运行myrunzoomroute.py文件来运行实验

目录结构说明：
（1）experimentResult文件夹：是用来存储产生的结果数据，如果原来安装的ndnSIM目录下没有该文件夹，需要重新创建
（2）topologies文件夹：里面是实验要用到的网络拓扑图
（3）ns-3文件夹：如果ndnSIM安装好了之后，这个ns-3文件夹是已经有的，我们只需要把ns-3文件夹里面的文件拷贝进去。注意ns-3文件夹下面的文件夹可能有的是已经存在的，这时候只需要把文件夹中的文件拷贝进去即可。
（4）ns-3/src/ndnSIM/model/fw文件夹对应的是我们基于熵权法实现的转发策略
