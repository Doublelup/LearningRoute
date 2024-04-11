这个文件夹是南京大学操作系统课程的课后实验部分  
框架代码来源：https://github.com/NJU-ProjectN/abstract-machine  
lab实现部分：./dir/  
update: 2024/3/19 lab0,lab1  
update: 2024/3/26 lab2(尚未测试）  
update: 2024/4/2  lab1在并发状态下有严重错误！😢  
update: 2024/4/2  紧急修复，看了之后发现是free时没有上锁，但由于print到终端的次序不能得到保证，所以暂时还不能充分验证多核情况下的正确性。  
update: 2024/4/3  经过重新检验，基本确定在多核下表现正常（不确定的地方是因为需要打印日志进行检验，所以在打印日志的时侯plock限制了程序运行的次序，不能排除表现正常是因为plock的限制！


