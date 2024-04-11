这个文件夹是南京大学操作系统课程的课后实验部分  
框架代码来源：https://github.com/NJU-ProjectN/abstract-machine  
lab实现部分：./dir/  
update: 2024/3/19 lab0,lab1  
### **notes（血的教训）**  
1.Makefile中生成规则改变了不会重新制作！比如为了能看源文件的信息方便调试，如果原来已经make过了，在规则中添加了-g不会重新生成.o，需要将原来生成的.o删掉后从新make  
呜呜，要严谨的学习makefile的规则，只有当依赖的文件修改日期比目标的修改日期新的时候才会重新生成！  
网站：https://seisman.github.io/how-to-write-makefile
