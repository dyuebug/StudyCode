# C++ Primer第五版学习笔记

学习环境：CentOS7 gcc4.8.5

以下内容需要更高的编译器版本支持：

- 正则表达式库

- 一些IO操纵符，比如hexfloat

可以使用[在线编译器](http://coliru.stacked-crooked.com/)编译运行相关代码。

---

## 第1章 开始

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习1.1](exercise_1_01.cpp "./ch01_Getting_Started/exercise_1_01.cpp")|[练习1.2](exercise_1_02.cpp "./ch01_Getting_Started/exercise_1_02.cpp")|[练习1.3](exercise_1_03.cpp "./ch01_Getting_Started/exercise_1_03.cpp")|[练习1.4](exercise_1_04.cpp "./ch01_Getting_Started/exercise_1_04.cpp")|[练习1.5](exercise_1_05.cpp "./ch01_Getting_Started/exercise_1_05.cpp")|[练习1.6](exercise_1_06.cpp "./ch01_Getting_Started/exercise_1_06.cpp")|
|[练习1.7](exercise_1_07.cpp "./ch01_Getting_Started/exercise_1_07.cpp")|[练习1.8](exercise_1_08.cpp "./ch01_Getting_Started/exercise_1_08.cpp")|[练习1.9](exercise_1_09.cpp "./ch01_Getting_Started/exercise_1_09.cpp")|[练习1.10](exercise_1_10.cpp "./ch01_Getting_Started/exercise_1_10.cpp")|[练习1.11](exercise_1_11.cpp "./ch01_Getting_Started/exercise_1_11.cpp")|[练习1.12](exercise_1_12.cpp "./ch01_Getting_Started/exercise_1_12.cpp")|
|[练习1.13](exercise_1_13.cpp "./ch01_Getting_Started/exercise_1_13.cpp")|[练习1.14](exercise_1_14.cpp "./ch01_Getting_Started/exercise_1_14.cpp")|[练习1.15](exercise_1_15.cpp "./ch01_Getting_Started/exercise_1_15.cpp")|[练习1.16](exercise_1_16.cpp "./ch01_Getting_Started/exercise_1_16.cpp")|[练习1.17](exercise_1_17.cpp "./ch01_Getting_Started/exercise_1_17.cpp")|[练习1.18](exercise_1_18.cpp "./ch01_Getting_Started/exercise_1_18.cpp")|
|[练习1.19](exercise_1_19.cpp "./ch01_Getting_Started/exercise_1_19.cpp")|[练习1.20](exercise_1_20.cpp "./ch01_Getting_Started/exercise_1_20.cpp")|[练习1.21](exercise_1_21.cpp "./ch01_Getting_Started/exercise_1_21.cpp")|[练习1.22](exercise_1_22.cpp "./ch01_Getting_Started/exercise_1_22.cpp")|[练习1.23](exercise_1_23.cpp "./ch01_Getting_Started/exercise_1_23.cpp")|[练习1.24](exercise_1_24.cpp "./ch01_Getting_Started/exercise_1_24.cpp")|
|[练习1.25](exercise_1_25.cpp "./ch01_Getting_Started/exercise_1_25.cpp")|

### 案例代码

- [C++中的注释使用（p8）](example_comments.cpp)

- [for语句（p11）](example_for.cpp)

- [if语句（p15）](example_if.cpp)

- [使用IO库（p5）](example_iostream.cpp)

- [最简单的C++程序（p2）](example_main.cpp)

- [while语句（p10）](example_while.cpp)

## 第2章 变量和基本类型

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习2.1](exercise_2_01.txt "./ch02_Variables_and_Basic_Types/exercise_2_01.txt")|[练习2.2](exercise_2_02.txt "./ch02_Variables_and_Basic_Types/exercise_2_02.txt")|[练习2.3](exercise_2_03.txt "./ch02_Variables_and_Basic_Types/exercise_2_03.txt")|[练习2.4](exercise_2_04.cpp "./ch02_Variables_and_Basic_Types/exercise_2_04.cpp")|[练习2.5](exercise_2_05.txt "./ch02_Variables_and_Basic_Types/exercise_2_05.txt")|[练习2.6](exercise_2_06.cpp "./ch02_Variables_and_Basic_Types/exercise_2_06.cpp")|
|[练习2.7](exercise_2_07.cpp "./ch02_Variables_and_Basic_Types/exercise_2_07.cpp")|[练习2.8](exercise_2_08.cpp "./ch02_Variables_and_Basic_Types/exercise_2_08.cpp")|[练习2.9](exercise_2_09.cpp "./ch02_Variables_and_Basic_Types/exercise_2_09.cpp")|[练习2.10](exercise_2_10.cpp "./ch02_Variables_and_Basic_Types/exercise_2_10.cpp")|[练习2.11](exercise_2_11.txt "./ch02_Variables_and_Basic_Types/exercise_2_11.txt")|[练习2.12](exercise_2_12.txt "./ch02_Variables_and_Basic_Types/exercise_2_12.txt")|
|[练习2.13](exercise_2_13.cpp "./ch02_Variables_and_Basic_Types/exercise_2_13.cpp")|[练习2.14](exercise_2_14.cpp "./ch02_Variables_and_Basic_Types/exercise_2_14.cpp")|[练习2.15](exercise_2_15.txt "./ch02_Variables_and_Basic_Types/exercise_2_15.txt")|[练习2.16](exercise_2_16.cpp "./ch02_Variables_and_Basic_Types/exercise_2_16.cpp")|[练习2.17](exercise_2_17.cpp "./ch02_Variables_and_Basic_Types/exercise_2_17.cpp")|[练习2.18](exercise_2_18.cpp "./ch02_Variables_and_Basic_Types/exercise_2_18.cpp")|
|[练习2.19](exercise_2_19.txt "./ch02_Variables_and_Basic_Types/exercise_2_19.txt")|[练习2.20](exercise_2_20.cpp "./ch02_Variables_and_Basic_Types/exercise_2_20.cpp")|[练习2.21](exercise_2_21.txt "./ch02_Variables_and_Basic_Types/exercise_2_21.txt")|[练习2.22](exercise_2_22.txt "./ch02_Variables_and_Basic_Types/exercise_2_22.txt")|[练习2.23](exercise_2_23.txt "./ch02_Variables_and_Basic_Types/exercise_2_23.txt")|[练习2.24](exercise_2_24.txt "./ch02_Variables_and_Basic_Types/exercise_2_24.txt")|
|[练习2.25](exercise_2_25.txt "./ch02_Variables_and_Basic_Types/exercise_2_25.txt")|[练习2.26](exercise_2_26.txt "./ch02_Variables_and_Basic_Types/exercise_2_26.txt")|[练习2.27](exercise_2_27.cpp "./ch02_Variables_and_Basic_Types/exercise_2_27.cpp")|[练习2.28](exercise_2_28.txt "./ch02_Variables_and_Basic_Types/exercise_2_28.txt")|[练习2.29](exercise_2_29.txt "./ch02_Variables_and_Basic_Types/exercise_2_29.txt")|[练习2.30](exercise_2_30.txt "./ch02_Variables_and_Basic_Types/exercise_2_30.txt")|
|[练习2.31](exercise_2_31.txt "./ch02_Variables_and_Basic_Types/exercise_2_31.txt")|[练习2.32](exercise_2_32.txt "./ch02_Variables_and_Basic_Types/exercise_2_32.txt")|[练习2.33](exercise_2_33.txt "./ch02_Variables_and_Basic_Types/exercise_2_33.txt")|[练习2.34](exercise_2_34.cpp "./ch02_Variables_and_Basic_Types/exercise_2_34.cpp")|[练习2.35](exercise_2_35.cpp "./ch02_Variables_and_Basic_Types/exercise_2_35.cpp")|[练习2.36](exercise_2_36.cpp "./ch02_Variables_and_Basic_Types/exercise_2_36.cpp")|
|[练习2.37](exercise_2_37.txt "./ch02_Variables_and_Basic_Types/exercise_2_37.txt")|[练习2.38](exercise_2_38.md "./ch02_Variables_and_Basic_Types/exercise_2_38.md")|[练习2.39](exercise_2_39.cpp "./ch02_Variables_and_Basic_Types/exercise_2_39.cpp")|[练习2.40](exercise_2_40.cpp "./ch02_Variables_and_Basic_Types/exercise_2_40.cpp")|[练习2.41](exercise_2_41.cpp "./ch02_Variables_and_Basic_Types/exercise_2_41.cpp")|[练习2.42](exercise_2_42.cpp "./ch02_Variables_and_Basic_Types/exercise_2_42.cpp")|

## 第3章 字符串、向量和数组

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习3.1](exercise_3_01.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_01.cpp")|[练习3.2](exercise_3_02.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_02.cpp")|[练习3.3](exercise_3_03.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_03.txt")|[练习3.4a](exercise_3_04a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_04a.cpp")|[练习3.4b](exercise_3_04b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_04b.cpp")|[练习3.5a](exercise_3_05a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_05a.cpp")|
|[练习3.5b](exercise_3_05b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_05b.cpp")|[练习3.6](exercise_3_06.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_06.cpp")|[练习3.7](exercise_3_07.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_07.cpp")|[练习3.8a](exercise_3_08a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_08a.cpp")|[练习3.8b](exercise_3_08b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_08b.cpp")|[练习3.9](exercise_3_09.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_09.txt")|
|[练习3.10](exercise_3_10.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_10.cpp")|[练习3.11](exercise_3_11.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_11.txt")|[练习3.12](exercise_3_12.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_12.txt")|[练习3.13](exercise_3_13.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_13.cpp")|[练习3.14](exercise_3_14.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_14.cpp")|[练习3.15](exercise_3_15.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_15.cpp")|
|[练习3.16](exercise_3_16.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_16.cpp")|[练习3.17](exercise_3_17.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_17.cpp")|[练习3.18](exercise_3_18.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_18.txt")|[练习3.19](exercise_3_19.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_19.cpp")|[练习3.20a](exercise_3_20a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_20a.cpp")|[练习3.20b](exercise_3_20b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_20b.cpp")|
|[练习3.21](exercise_3_21.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_21.cpp")|[练习3.22](exercise_3_22.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_22.cpp")|[练习3.23](exercise_3_23.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_23.cpp")|[练习3.24a](exercise_3_24a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_24a.cpp")|[练习3.24b](exercise_3_24b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_24b.cpp")|[练习3.25](exercise_3_25.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_25.cpp")|
|[练习3.26](exercise_3_26.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_26.txt")|[练习3.27](exercise_3_27.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_27.txt")|[练习3.28](exercise_3_28.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_28.cpp")|[练习3.29](exercise_3_29.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_29.txt")|[练习3.30](exercise_3_30.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_30.txt")|[练习3.31](exercise_3_31.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_31.cpp")|
|[练习3.32a](exercise_3_32a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_32a.cpp")|[练习3.32b](exercise_3_32b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_32b.cpp")|[练习3.33](exercise_3_33.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_33.txt")|[练习3.34](exercise_3_34.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_34.txt")|[练习3.35](exercise_3_35.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_35.cpp")|[练习3.36a](exercise_3_36a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_36a.cpp")|
|[练习3.36b](exercise_3_36b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_36b.cpp")|[练习3.37](exercise_3_37.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_37.cpp")|[练习3.38](exercise_3_38.txt "./ch03_Strings_Vectors_and_Arrays/exercise_3_38.txt")|[练习3.39a](exercise_3_39a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_39a.cpp")|[练习3.39b](exercise_3_39b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_39b.cpp")|[练习3.40](exercise_3_40.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_40.cpp")|
|[练习3.41](exercise_3_41.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_41.cpp")|[练习3.42](exercise_3_42.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_42.cpp")|[练习3.43a](exercise_3_43a.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_43a.cpp")|[练习3.43b](exercise_3_43b.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_43b.cpp")|[练习3.43c](exercise_3_43c.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_43c.cpp")|[练习3.44](exercise_3_44.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_44.cpp")|
|[练习3.45](exercise_3_45.cpp "./ch03_Strings_Vectors_and_Arrays/exercise_3_45.cpp")|

### 案例代码

- [使用getline读取一整行（p78）](example_getline.cpp)

- [一维数组模拟二维数组](example_normal_array_simulate_dimensional_array.cpp)

- [向vector对象中添加元素（p90）](example_push_back.cpp)

- [使用基于范围for语句处理每个字符（p82）](example_range_for.cpp)

- [读取未知数量的string对象（p78）](example_read_string.cpp)

- [迭代器运算符（p96）](example_using_iterators.cpp)

- [使用下标执行迭代（p85）](example_using_subscript.cpp)

## 第4章 表达式

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习4.1](exercise_4_01.txt "./ch04_Expressions/exercise_4_01.txt")|[练习4.2](exercise_4_02.txt "./ch04_Expressions/exercise_4_02.txt")|[练习4.3](exercise_4_03.txt "./ch04_Expressions/exercise_4_03.txt")|[练习4.4](exercise_4_04.cpp "./ch04_Expressions/exercise_4_04.cpp")|[练习4.5](exercise_4_05.txt "./ch04_Expressions/exercise_4_05.txt")|[练习4.6](exercise_4_06.txt "./ch04_Expressions/exercise_4_06.txt")|
|[练习4.7](exercise_4_07.cpp "./ch04_Expressions/exercise_4_07.cpp")|[练习4.8](exercise_4_08.txt "./ch04_Expressions/exercise_4_08.txt")|[练习4.9](exercise_4_09.txt "./ch04_Expressions/exercise_4_09.txt")|[练习4.10](exercise_4_10.cpp "./ch04_Expressions/exercise_4_10.cpp")|[练习4.11](exercise_4_11.txt "./ch04_Expressions/exercise_4_11.txt")|[练习4.12](exercise_4_12.txt "./ch04_Expressions/exercise_4_12.txt")|
|[练习4.13](exercise_4_13.cpp "./ch04_Expressions/exercise_4_13.cpp")|[练习4.14](exercise_4_14.txt "./ch04_Expressions/exercise_4_14.txt")|[练习4.15](exercise_4_15.txt "./ch04_Expressions/exercise_4_15.txt")|[练习4.16](exercise_4_16.cpp "./ch04_Expressions/exercise_4_16.cpp")|[练习4.17](exercise_4_17.txt "./ch04_Expressions/exercise_4_17.txt")|[练习4.18](exercise_4_18.txt "./ch04_Expressions/exercise_4_18.txt")|
|[练习4.19](exercise_4_19.txt "./ch04_Expressions/exercise_4_19.txt")|[练习4.20](exercise_4_20.txt "./ch04_Expressions/exercise_4_20.txt")|[练习4.21](exercise_4_21.cpp "./ch04_Expressions/exercise_4_21.cpp")|[练习4.22a](exercise_4_22a.cpp "./ch04_Expressions/exercise_4_22a.cpp")|[练习4.22b](exercise_4_22b.cpp "./ch04_Expressions/exercise_4_22b.cpp")|[练习4.23](exercise_4_23.cpp "./ch04_Expressions/exercise_4_23.cpp")|
|[练习4.24](exercise_4_24.txt "./ch04_Expressions/exercise_4_24.txt")|[练习4.25](exercise_4_25.cpp "./ch04_Expressions/exercise_4_25.cpp")|[练习4.26](exercise_4_26.txt "./ch04_Expressions/exercise_4_26.txt")|[练习4.27](exercise_4_27.cpp "./ch04_Expressions/exercise_4_27.cpp")|[练习4.28](exercise_4_28.cpp "./ch04_Expressions/exercise_4_28.cpp")|[练习4.29](exercise_4_29.cpp "./ch04_Expressions/exercise_4_29.cpp")|
|[练习4.30](exercise_4_30.txt "./ch04_Expressions/exercise_4_30.txt")|[练习4.31](exercise_4_31.cpp "./ch04_Expressions/exercise_4_31.cpp")|[练习4.32](exercise_4_32.txt "./ch04_Expressions/exercise_4_32.txt")|[练习4.33](exercise_4_33.txt "./ch04_Expressions/exercise_4_33.txt")|[练习4.34](exercise_4_34.txt "./ch04_Expressions/exercise_4_34.txt")|[练习4.35](exercise_4_35.txt "./ch04_Expressions/exercise_4_35.txt")|
|[练习4.36](exercise_4_36.txt "./ch04_Expressions/exercise_4_36.txt")|[练习4.37](exercise_4_37.txt "./ch04_Expressions/exercise_4_37.txt")|[练习4.38](exercise_4_38.txt "./ch04_Expressions/exercise_4_38.txt")|

### 案例代码

- [逗号运算符（p140）](example_comma_operator.cpp)

- [在一条语句中混用解引用和递增运算符（p132）](example_increment_operator.cpp)

## 第5章 语句

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习5.1](exercise_5_01.txt "./ch05_Statements/exercise_5_01.txt")|[练习5.2](exercise_5_02.txt "./ch05_Statements/exercise_5_02.txt")|[练习5.3](exercise_5_03.cpp "./ch05_Statements/exercise_5_03.cpp")|[练习5.4](exercise_5_04.txt "./ch05_Statements/exercise_5_04.txt")|[练习5.5](exercise_5_05.cpp "./ch05_Statements/exercise_5_05.cpp")|[练习5.6](exercise_5_06.cpp "./ch05_Statements/exercise_5_06.cpp")|
|[练习5.7](exercise_5_07.txt "./ch05_Statements/exercise_5_07.txt")|[练习5.8](exercise_5_08.txt "./ch05_Statements/exercise_5_08.txt")|[练习5.9](exercise_5_09.cpp "./ch05_Statements/exercise_5_09.cpp")|[练习5.10](exercise_5_10.cpp "./ch05_Statements/exercise_5_10.cpp")|[练习5.11](exercise_5_11.cpp "./ch05_Statements/exercise_5_11.cpp")|[练习5.12](exercise_5_12.cpp "./ch05_Statements/exercise_5_12.cpp")|
|[练习5.13](exercise_5_13.txt "./ch05_Statements/exercise_5_13.txt")|[练习5.14](exercise_5_14.cpp "./ch05_Statements/exercise_5_14.cpp")|[练习5.15](exercise_5_15.txt "./ch05_Statements/exercise_5_15.txt")|[练习5.16a](exercise_5_16a.cpp "./ch05_Statements/exercise_5_16a.cpp")|[练习5.16b](exercise_5_16b.cpp "./ch05_Statements/exercise_5_16b.cpp")|[练习5.16c](exercise_5_16c.cpp "./ch05_Statements/exercise_5_16c.cpp")|
|[练习5.16d](exercise_5_16d.cpp "./ch05_Statements/exercise_5_16d.cpp")|[练习5.17](exercise_5_17.cpp "./ch05_Statements/exercise_5_17.cpp")|[练习5.18](exercise_5_18.txt "./ch05_Statements/exercise_5_18.txt")|[练习5.19](exercise_5_19.cpp "./ch05_Statements/exercise_5_19.cpp")|[练习5.20](exercise_5_20.cpp "./ch05_Statements/exercise_5_20.cpp")|[练习5.21](exercise_5_21.cpp "./ch05_Statements/exercise_5_21.cpp")|
|[练习5.22](exercise_5_22.txt "./ch05_Statements/exercise_5_22.txt")|[练习5.23](exercise_5_23.cpp "./ch05_Statements/exercise_5_23.cpp")|[练习5.24](exercise_5_24.cpp "./ch05_Statements/exercise_5_24.cpp")|[练习5.25](exercise_5_25.cpp "./ch05_Statements/exercise_5_25.cpp")|

### 案例代码

- [do while语句（p169）](example_do_while.cpp)

- [嵌套if语句（p157）](example_if_statement.cpp)

## 第6章 函数

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习6.1](exercise_6_01.txt "./ch06_Functions/exercise_6_01.txt")|[练习6.2](exercise_6_02.txt "./ch06_Functions/exercise_6_02.txt")|[练习6.3](exercise_6_03.cpp "./ch06_Functions/exercise_6_03.cpp")|[练习6.4](exercise_6_04.cpp "./ch06_Functions/exercise_6_04.cpp")|[练习6.5](exercise_6_05.cpp "./ch06_Functions/exercise_6_05.cpp")|[练习6.6](exercise_6_06.cpp "./ch06_Functions/exercise_6_06.cpp")|
|[练习6.7](exercise_6_07.cpp "./ch06_Functions/exercise_6_07.cpp")|[练习6.8](exercise_6_08.txt "./ch06_Functions/exercise_6_08.txt")|[练习6.9](ch06_Functions/exercise_6_09 "./ch06_Functions/exercise_6_09")|[练习6.10](exercise_6_10.cpp "./ch06_Functions/exercise_6_10.cpp")|[练习6.11](exercise_6_11.cpp "./ch06_Functions/exercise_6_11.cpp")|[练习6.12](exercise_6_12.cpp "./ch06_Functions/exercise_6_12.cpp")|
|[练习6.13](exercise_6_13.txt "./ch06_Functions/exercise_6_13.txt")|[练习6.14](exercise_6_14.txt "./ch06_Functions/exercise_6_14.txt")|[练习6.15](exercise_6_15.txt "./ch06_Functions/exercise_6_15.txt")|[练习6.16](exercise_6_16.md "./ch06_Functions/exercise_6_16.md")|[练习6.17](exercise_6_17.cpp "./ch06_Functions/exercise_6_17.cpp")|[练习6.18](exercise_6_18.md "./ch06_Functions/exercise_6_18.md")|
|[练习6.19](exercise_6_19.md "./ch06_Functions/exercise_6_19.md")|[练习6.20](exercise_6_20.md "./ch06_Functions/exercise_6_20.md")|[练习6.21](exercise_6_21.cpp "./ch06_Functions/exercise_6_21.cpp")|[练习6.22](exercise_6_22.cpp "./ch06_Functions/exercise_6_22.cpp")|[练习6.23](exercise_6_23.cpp "./ch06_Functions/exercise_6_23.cpp")|[练习6.24](exercise_6_24.cpp "./ch06_Functions/exercise_6_24.cpp")|
|[练习6.25](exercise_6_25.cpp "./ch06_Functions/exercise_6_25.cpp")|[练习6.26](exercise_6_26.cpp "./ch06_Functions/exercise_6_26.cpp")|[练习6.27](exercise_6_27.cpp "./ch06_Functions/exercise_6_27.cpp")|[练习6.28](exercise_6_28.md "./ch06_Functions/exercise_6_28.md")|[练习6.29](exercise_6_29.md "./ch06_Functions/exercise_6_29.md")|[练习6.30](exercise_6_30.cpp "./ch06_Functions/exercise_6_30.cpp")|
|[练习6.31](exercise_6_31.md "./ch06_Functions/exercise_6_31.md")|[练习6.32](exercise_6_32.md "./ch06_Functions/exercise_6_32.md")|[练习6.33](exercise_6_33.cpp "./ch06_Functions/exercise_6_33.cpp")|[练习6.34](exercise_6_34.cpp "./ch06_Functions/exercise_6_34.cpp")|[练习6.35](exercise_6_35.md "./ch06_Functions/exercise_6_35.md")|[练习6.36](exercise_6_36.cpp "./ch06_Functions/exercise_6_36.cpp")|
|[练习6.37](exercise_6_37.cpp "./ch06_Functions/exercise_6_37.cpp")|[练习6.38](exercise_6_38.cpp "./ch06_Functions/exercise_6_38.cpp")|[练习6.39](exercise_6_39.md "./ch06_Functions/exercise_6_39.md")|[练习6.40](exercise_6_40.md "./ch06_Functions/exercise_6_40.md")|[练习6.41](exercise_6_41.md "./ch06_Functions/exercise_6_41.md")|[练习6.42](exercise_6_42.cpp "./ch06_Functions/exercise_6_42.cpp")|
|[练习6.43](exercise_6_43.md "./ch06_Functions/exercise_6_43.md")|[练习6.44](exercise_6_44.cpp "./ch06_Functions/exercise_6_44.cpp")|[练习6.45](exercise_6_45.md "./ch06_Functions/exercise_6_45.md")|[练习6.46](exercise_6_46.cpp "./ch06_Functions/exercise_6_46.cpp")|[练习6.47](exercise_6_47.cpp "./ch06_Functions/exercise_6_47.cpp")|[练习6.48](exercise_6_48.md "./ch06_Functions/exercise_6_48.md")|
|[练习6.49](exercise_6_49.md "./ch06_Functions/exercise_6_49.md")|[练习6.50](exercise_6_50.md "./ch06_Functions/exercise_6_50.md")|[练习6.51](exercise_6_51.cpp "./ch06_Functions/exercise_6_51.cpp")|[练习6.52](exercise_6_52.md "./ch06_Functions/exercise_6_52.md")|[练习6.53](exercise_6_53.md "./ch06_Functions/exercise_6_53.md")|[练习6.54](exercise_6_54.cpp "./ch06_Functions/exercise_6_54.cpp")|
|[练习6.55](exercise_6_55.cpp "./ch06_Functions/exercise_6_55.cpp")|[练习6.56](exercise_6_56.cpp "./ch06_Functions/exercise_6_56.cpp")|

### 案例代码

- [数组形参（p193）](example_array_param.cpp)

- [const_cast和重载（p209）](example_const_cast_and_overloading.cpp)

- [无返回值函数（p200）](example_functions_with_no_return_value.cpp)

- [initializer_list形参（可变形参，p198）](example_initializer_list_param.cpp)

- [内联函数可避免函数调用开销（p213）](example_inline_functions.cpp)

- [传递多维数组给形参（p195）](example_passing_a_multidimensional_array.cpp)

- [使用预处理变量输出错误信息（p216）](example_preprocessor_variable.cpp)

- [递归（p204）](example_recursion.cpp)

- [主函数main的返回值（p203）](example_return_from_main.cpp)

- [返回数组指针（使用decltype，p206）](example_returning_a_pointer_to_an_array.cpp)

- [使用引用传递避免拷贝（p189）](example_using_ref_to_avoid_cp.cpp)

- [使用引用形参返回额外信息（p189）](example_using_ref_to_return_info.cpp)

## 第7章 类

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习7.1](exercise_7_01.cpp "./ch07_Classes/exercise_7_01.cpp")|[练习7.2](exercise_7_02.cpp "./ch07_Classes/exercise_7_02.cpp")|[练习7.3](exercise_7_03.cpp "./ch07_Classes/exercise_7_03.cpp")|[练习7.4](exercise_7_04.cpp "./ch07_Classes/exercise_7_04.cpp")|[练习7.5](exercise_7_05.cpp "./ch07_Classes/exercise_7_05.cpp")|[练习7.6](exercise_7_06.cpp "./ch07_Classes/exercise_7_06.cpp")|
|[练习7.7](exercise_7_07.cpp "./ch07_Classes/exercise_7_07.cpp")|[练习7.8](exercise_7_08.md "./ch07_Classes/exercise_7_08.md")|[练习7.9](exercise_7_09.cpp "./ch07_Classes/exercise_7_09.cpp")|[练习7.10](exercise_7_10.md "./ch07_Classes/exercise_7_10.md")|[练习7.11](exercise_7_11.cpp "./ch07_Classes/exercise_7_11.cpp")|[练习7.12](exercise_7_12.cpp "./ch07_Classes/exercise_7_12.cpp")|
|[练习7.13](exercise_7_13.cpp "./ch07_Classes/exercise_7_13.cpp")|[练习7.14](exercise_7_14.md "./ch07_Classes/exercise_7_14.md")|[练习7.15](exercise_7_15.cpp "./ch07_Classes/exercise_7_15.cpp")|[练习7.16](exercise_7_16.md "./ch07_Classes/exercise_7_16.md")|[练习7.17](exercise_7_17.md "./ch07_Classes/exercise_7_17.md")|[练习7.18](exercise_7_18.md "./ch07_Classes/exercise_7_18.md")|
|[练习7.19](exercise_7_19.cpp "./ch07_Classes/exercise_7_19.cpp")|[练习7.20](exercise_7_20.md "./ch07_Classes/exercise_7_20.md")|[练习7.21](exercise_7_21.md "./ch07_Classes/exercise_7_21.md")|[练习7.22](exercise_7_22.cpp "./ch07_Classes/exercise_7_22.cpp")|[练习7.23](exercise_7_23.cpp "./ch07_Classes/exercise_7_23.cpp")|[练习7.24](exercise_7_24.cpp "./ch07_Classes/exercise_7_24.cpp")|
|[练习7.25](exercise_7_25.md "./ch07_Classes/exercise_7_25.md")|[练习7.26](exercise_7_26.md "./ch07_Classes/exercise_7_26.md")|[练习7.27](exercise_7_27.cpp "./ch07_Classes/exercise_7_27.cpp")|[练习7.28](exercise_7_28.md "./ch07_Classes/exercise_7_28.md")|[练习7.29](exercise_7_29.cpp "./ch07_Classes/exercise_7_29.cpp")|[练习7.30](exercise_7_30.md "./ch07_Classes/exercise_7_30.md")|
|[练习7.31](exercise_7_31.cpp "./ch07_Classes/exercise_7_31.cpp")|[练习7.32](exercise_7_32.cpp "./ch07_Classes/exercise_7_32.cpp")|[练习7.33](exercise_7_33.md "./ch07_Classes/exercise_7_33.md")|[练习7.34](exercise_7_34.md "./ch07_Classes/exercise_7_34.md")|[练习7.35](exercise_7_35.cpp "./ch07_Classes/exercise_7_35.cpp")|[练习7.36](exercise_7_36.cpp "./ch07_Classes/exercise_7_36.cpp")|
|[练习7.37](exercise_7_37.md "./ch07_Classes/exercise_7_37.md")|[练习7.38](exercise_7_38.cpp "./ch07_Classes/exercise_7_38.cpp")|[练习7.39](exercise_7_39.md "./ch07_Classes/exercise_7_39.md")|[练习7.40](exercise_7_40.cpp "./ch07_Classes/exercise_7_40.cpp")|[练习7.41](exercise_7_41.cpp "./ch07_Classes/exercise_7_41.cpp")|[练习7.42](exercise_7_42.cpp "./ch07_Classes/exercise_7_42.cpp")|
|[练习7.43](exercise_7_43.cpp "./ch07_Classes/exercise_7_43.cpp")|[练习7.44](exercise_7_44.md "./ch07_Classes/exercise_7_44.md")|[练习7.45](exercise_7_45.md "./ch07_Classes/exercise_7_45.md")|[练习7.46](exercise_7_46.md "./ch07_Classes/exercise_7_46.md")|[练习7.47](exercise_7_47.md "./ch07_Classes/exercise_7_47.md")|[练习7.48](exercise_7_48.md "./ch07_Classes/exercise_7_48.md")|
|[练习7.49](exercise_7_49.md "./ch07_Classes/exercise_7_49.md")|[练习7.50](exercise_7_50.md "./ch07_Classes/exercise_7_50.md")|[练习7.51](exercise_7_51.md "./ch07_Classes/exercise_7_51.md")|[练习7.52](exercise_7_52.md "./ch07_Classes/exercise_7_52.md")|[练习7.53](exercise_7_53.md "./ch07_Classes/exercise_7_53.md")|[练习7.54](exercise_7_54.md "./ch07_Classes/exercise_7_54.md")|
|[练习7.55](exercise_7_55.md "./ch07_Classes/exercise_7_55.md")|[练习7.56](exercise_7_56.md "./ch07_Classes/exercise_7_56.md")|[练习7.57](exercise_7_57.md "./ch07_Classes/exercise_7_57.md")|[练习7.58](exercise_7_58.md "./ch07_Classes/exercise_7_58.md")|

### 案例代码

- [类的静态成员（p269）](ch07_Classes/example_Account)

- [字面值常量类（p267）](ch07_Classes/example_literal_classes)

- [Person类的设计与实现](example_Person.cpp)

- [Sales_data案例（自p228始）](ch07_Classes/example_Sales_data)

- [窗口类的设计与实现(自p243始)](ch07_Classes/example_Screen)

## 第8章 IO库

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习8.1](exercise_8_01.cpp "./ch08_The_IO_Library/exercise_8_01.cpp")|[练习8.2](exercise_8_02.md "./ch08_The_IO_Library/exercise_8_02.md")|[练习8.3](exercise_8_03.md "./ch08_The_IO_Library/exercise_8_03.md")|[练习8.4](exercise_8_04.cpp "./ch08_The_IO_Library/exercise_8_04.cpp")|[练习8.5](exercise_8_05.cpp "./ch08_The_IO_Library/exercise_8_05.cpp")|[练习8.6](exercise_8_06.cpp "./ch08_The_IO_Library/exercise_8_06.cpp")|
|[练习8.7](exercise_8_07.cpp "./ch08_The_IO_Library/exercise_8_07.cpp")|[练习8.8](exercise_8_08.cpp "./ch08_The_IO_Library/exercise_8_08.cpp")|[练习8.9](exercise_8_09.cpp "./ch08_The_IO_Library/exercise_8_09.cpp")|[练习8.10](exercise_8_10.cpp "./ch08_The_IO_Library/exercise_8_10.cpp")|[练习8.11](exercise_8_11.cpp "./ch08_The_IO_Library/exercise_8_11.cpp")|[练习8.12](exercise_8_12.md "./ch08_The_IO_Library/exercise_8_12.md")|
|[练习8.13](exercise_8_13.cpp "./ch08_The_IO_Library/exercise_8_13.cpp")|[练习8.14](exercise_8_14.md "./ch08_The_IO_Library/exercise_8_14.md")|

### 案例代码

- [使用fstream代替iostream&读取Sales_data（p284）](example_fstream.cpp)

## 第9章 顺序容器

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习9.1](exercise_9_01.md "./ch09_Sequential_Containers/exercise_9_01.md")|[练习9.2](exercise_9_02.cpp "./ch09_Sequential_Containers/exercise_9_02.cpp")|[练习9.3](exercise_9_03.md "./ch09_Sequential_Containers/exercise_9_03.md")|[练习9.4](exercise_9_04.cpp "./ch09_Sequential_Containers/exercise_9_04.cpp")|[练习9.5](exercise_9_05.cpp "./ch09_Sequential_Containers/exercise_9_05.cpp")|[练习9.6](exercise_9_06.md "./ch09_Sequential_Containers/exercise_9_06.md")|
|[练习9.7](exercise_9_07.md "./ch09_Sequential_Containers/exercise_9_07.md")|[练习9.8](exercise_9_08.md "./ch09_Sequential_Containers/exercise_9_08.md")|[练习9.9](exercise_9_09.md "./ch09_Sequential_Containers/exercise_9_09.md")|[练习9.10](exercise_9_10.md "./ch09_Sequential_Containers/exercise_9_10.md")|[练习9.11](exercise_9_11.cpp "./ch09_Sequential_Containers/exercise_9_11.cpp")|[练习9.12](exercise_9_12.md "./ch09_Sequential_Containers/exercise_9_12.md")|
|[练习9.13](exercise_9_13.cpp "./ch09_Sequential_Containers/exercise_9_13.cpp")|[练习9.14](exercise_9_14.cpp "./ch09_Sequential_Containers/exercise_9_14.cpp")|[练习9.15](exercise_9_15.cpp "./ch09_Sequential_Containers/exercise_9_15.cpp")|[练习9.16](exercise_9_16.cpp "./ch09_Sequential_Containers/exercise_9_16.cpp")|[练习9.17](exercise_9_17.md "./ch09_Sequential_Containers/exercise_9_17.md")|[练习9.18](exercise_9_18.cpp "./ch09_Sequential_Containers/exercise_9_18.cpp")|
|[练习9.19](exercise_9_19.cpp "./ch09_Sequential_Containers/exercise_9_19.cpp")|[练习9.20](exercise_9_20.cpp "./ch09_Sequential_Containers/exercise_9_20.cpp")|[练习9.21](exercise_9_21.md "./ch09_Sequential_Containers/exercise_9_21.md")|[练习9.22](exercise_9_22.md "./ch09_Sequential_Containers/exercise_9_22.md")|[练习9.23](exercise_9_23.md "./ch09_Sequential_Containers/exercise_9_23.md")|[练习9.24](exercise_9_24.cpp "./ch09_Sequential_Containers/exercise_9_24.cpp")|
|[练习9.25](exercise_9_25.cpp "./ch09_Sequential_Containers/exercise_9_25.cpp")|[练习9.26](exercise_9_26.cpp "./ch09_Sequential_Containers/exercise_9_26.cpp")|[练习9.27](exercise_9_27.cpp "./ch09_Sequential_Containers/exercise_9_27.cpp")|[练习9.28](exercise_9_28.cpp "./ch09_Sequential_Containers/exercise_9_28.cpp")|[练习9.29](exercise_9_29.md "./ch09_Sequential_Containers/exercise_9_29.md")|[练习9.30](exercise_9_30.md "./ch09_Sequential_Containers/exercise_9_30.md")|
|[练习9.31](exercise_9_31.cpp "./ch09_Sequential_Containers/exercise_9_31.cpp")|[练习9.32](exercise_9_32.md "./ch09_Sequential_Containers/exercise_9_32.md")|[练习9.33](exercise_9_33.cpp "./ch09_Sequential_Containers/exercise_9_33.cpp")|[练习9.34](exercise_9_34.cpp "./ch09_Sequential_Containers/exercise_9_34.cpp")|[练习9.35](exercise_9_35.md "./ch09_Sequential_Containers/exercise_9_35.md")|[练习9.36](exercise_9_36.md "./ch09_Sequential_Containers/exercise_9_36.md")|
|[练习9.37](exercise_9_37.md "./ch09_Sequential_Containers/exercise_9_37.md")|[练习9.38](exercise_9_38.cpp "./ch09_Sequential_Containers/exercise_9_38.cpp")|[练习9.39](exercise_9_39.md "./ch09_Sequential_Containers/exercise_9_39.md")|[练习9.40](exercise_9_40.cpp "./ch09_Sequential_Containers/exercise_9_40.cpp")|[练习9.41](exercise_9_41.cpp "./ch09_Sequential_Containers/exercise_9_41.cpp")|[练习9.42](exercise_9_42.md "./ch09_Sequential_Containers/exercise_9_42.md")|
|[练习9.43](exercise_9_43.cpp "./ch09_Sequential_Containers/exercise_9_43.cpp")|[练习9.44](exercise_9_44.cpp "./ch09_Sequential_Containers/exercise_9_44.cpp")|[练习9.45](exercise_9_45.cpp "./ch09_Sequential_Containers/exercise_9_45.cpp")|[练习9.46](exercise_9_46.cpp "./ch09_Sequential_Containers/exercise_9_46.cpp")|[练习9.47](exercise_9_47.cpp "./ch09_Sequential_Containers/exercise_9_47.cpp")|[练习9.48](exercise_9_48.cpp "./ch09_Sequential_Containers/exercise_9_48.cpp")|
|[练习9.49](exercise_9_49.cpp "./ch09_Sequential_Containers/exercise_9_49.cpp")|[练习9.50](exercise_9_50.cpp "./ch09_Sequential_Containers/exercise_9_50.cpp")|[练习9.51](exercise_9_51.cpp "./ch09_Sequential_Containers/exercise_9_51.cpp")|[练习9.52](exercise_9_52.cpp "./ch09_Sequential_Containers/exercise_9_52.cpp")|

### 案例代码

- [改变容器的循环程序（p316）](example_change_container_in_loop.cpp)

- [从容器内部删除一个元素（p312）](example_erase.cpp)

- [forward_list操作（p313）](example_forward_list.cpp)

- [使用insert的返回值（p308）](example_insert.cpp)

- [迭代器范围iterator range（p296）](example_iterator_range.cpp)

## 第10章 泛型算法

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习10.1](exercise_10_01.cpp "./ch10_Generic_Algorithms/exercise_10_01.cpp")|[练习10.2](exercise_10_02.cpp "./ch10_Generic_Algorithms/exercise_10_02.cpp")|[练习10.3](exercise_10_03.cpp "./ch10_Generic_Algorithms/exercise_10_03.cpp")|[练习10.4](exercise_10_04.cpp "./ch10_Generic_Algorithms/exercise_10_04.cpp")|[练习10.5](exercise_10_05.md "./ch10_Generic_Algorithms/exercise_10_05.md")|[练习10.6](exercise_10_06.cpp "./ch10_Generic_Algorithms/exercise_10_06.cpp")|
|[练习10.7](exercise_10_07.md "./ch10_Generic_Algorithms/exercise_10_07.md")|[练习10.8](exercise_10_08.md "./ch10_Generic_Algorithms/exercise_10_08.md")|[练习10.9](exercise_10_09.cpp "./ch10_Generic_Algorithms/exercise_10_09.cpp")|[练习10.10](exercise_10_10.md "./ch10_Generic_Algorithms/exercise_10_10.md")|[练习10.11](exercise_10_11.cpp "./ch10_Generic_Algorithms/exercise_10_11.cpp")|[练习10.12](exercise_10_12.cpp "./ch10_Generic_Algorithms/exercise_10_12.cpp")|
|[练习10.13](exercise_10_13.cpp "./ch10_Generic_Algorithms/exercise_10_13.cpp")|[练习10.14](exercise_10_14.cpp "./ch10_Generic_Algorithms/exercise_10_14.cpp")|[练习10.15](exercise_10_15.cpp "./ch10_Generic_Algorithms/exercise_10_15.cpp")|[练习10.16](exercise_10_16.cpp "./ch10_Generic_Algorithms/exercise_10_16.cpp")|[练习10.17](exercise_10_17.cpp "./ch10_Generic_Algorithms/exercise_10_17.cpp")|[练习10.18](exercise_10_18.cpp "./ch10_Generic_Algorithms/exercise_10_18.cpp")|
|[练习10.19](exercise_10_19.cpp "./ch10_Generic_Algorithms/exercise_10_19.cpp")|[练习10.20](exercise_10_20.cpp "./ch10_Generic_Algorithms/exercise_10_20.cpp")|[练习10.21](exercise_10_21.cpp "./ch10_Generic_Algorithms/exercise_10_21.cpp")|[练习10.22](exercise_10_22.cpp "./ch10_Generic_Algorithms/exercise_10_22.cpp")|[练习10.23](exercise_10_23.md "./ch10_Generic_Algorithms/exercise_10_23.md")|[练习10.24](exercise_10_24.cpp "./ch10_Generic_Algorithms/exercise_10_24.cpp")|
|[练习10.25](exercise_10_25.cpp "./ch10_Generic_Algorithms/exercise_10_25.cpp")|[练习10.26](exercise_10_26.md "./ch10_Generic_Algorithms/exercise_10_26.md")|[练习10.27](exercise_10_27.cpp "./ch10_Generic_Algorithms/exercise_10_27.cpp")|[练习10.28](exercise_10_28.cpp "./ch10_Generic_Algorithms/exercise_10_28.cpp")|[练习10.29](exercise_10_29.cpp "./ch10_Generic_Algorithms/exercise_10_29.cpp")|[练习10.30](exercise_10_30.cpp "./ch10_Generic_Algorithms/exercise_10_30.cpp")|
|[练习10.31](exercise_10_31.cpp "./ch10_Generic_Algorithms/exercise_10_31.cpp")|[练习10.32](exercise_10_32.cpp "./ch10_Generic_Algorithms/exercise_10_32.cpp")|[练习10.33](exercise_10_33.cpp "./ch10_Generic_Algorithms/exercise_10_33.cpp")|[练习10.34](exercise_10_34.cpp "./ch10_Generic_Algorithms/exercise_10_34.cpp")|[练习10.35](exercise_10_35.cpp "./ch10_Generic_Algorithms/exercise_10_35.cpp")|[练习10.36](exercise_10_36.cpp "./ch10_Generic_Algorithms/exercise_10_36.cpp")|
|[练习10.37](exercise_10_37.cpp "./ch10_Generic_Algorithms/exercise_10_37.cpp")|[练习10.38](exercise_10_38.md "./ch10_Generic_Algorithms/exercise_10_38.md")|[练习10.39](exercise_10_39.md "./ch10_Generic_Algorithms/exercise_10_39.md")|[练习10.40](exercise_10_40.md "./ch10_Generic_Algorithms/exercise_10_40.md")|[练习10.41](exercise_10_41.md "./ch10_Generic_Algorithms/exercise_10_41.md")|[练习10.42](exercise_10_42.cpp "./ch10_Generic_Algorithms/exercise_10_42.cpp")|

### 案例代码

- [介绍back_inserter(p341)](example_back_inserter.cpp)

- [istream_iterator操作（p359）](example_istream_iterator.cpp)

- [使用lambda案例（p349）](example_lambda.cpp)

- [ostream_iterator操作（p361）](example_ostream_iterator.cpp)

- [stable_partition，将序列分为两组，满足谓词的放在前面一组](example_stable_partition.cpp)

- [stable_sort（p345）](example_stable_sort.cpp)

- [sort和unique函数，消除重复单词（p343）](example_unique.cpp)

## 第11章 关联容器

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习11.1](exercise_11_01.md "./ch11_Associative_Containers/exercise_11_01.md")|[练习11.2](exercise_11_02.md "./ch11_Associative_Containers/exercise_11_02.md")|[练习11.3](exercise_11_03.cpp "./ch11_Associative_Containers/exercise_11_03.cpp")|[练习11.4](exercise_11_04.cpp "./ch11_Associative_Containers/exercise_11_04.cpp")|[练习11.5](exercise_11_05.md "./ch11_Associative_Containers/exercise_11_05.md")|[练习11.6](exercise_11_06.md "./ch11_Associative_Containers/exercise_11_06.md")|
|[练习11.7](exercise_11_07.cpp "./ch11_Associative_Containers/exercise_11_07.cpp")|[练习11.8](exercise_11_08.cpp "./ch11_Associative_Containers/exercise_11_08.cpp")|[练习11.9](exercise_11_09.cpp "./ch11_Associative_Containers/exercise_11_09.cpp")|[练习11.10](exercise_11_10.md "./ch11_Associative_Containers/exercise_11_10.md")|[练习11.11](exercise_11_11.cpp "./ch11_Associative_Containers/exercise_11_11.cpp")|[练习11.12](exercise_11_12.cpp "./ch11_Associative_Containers/exercise_11_12.cpp")|
|[练习11.13](exercise_11_13.cpp "./ch11_Associative_Containers/exercise_11_13.cpp")|[练习11.14](exercise_11_14.cpp "./ch11_Associative_Containers/exercise_11_14.cpp")|[练习11.15](exercise_11_15.md "./ch11_Associative_Containers/exercise_11_15.md")|[练习11.16](exercise_11_16.md "./ch11_Associative_Containers/exercise_11_16.md")|[练习11.17](exercise_11_17.cpp "./ch11_Associative_Containers/exercise_11_17.cpp")|[练习11.18](exercise_11_18.md "./ch11_Associative_Containers/exercise_11_18.md")|
|[练习11.19](exercise_11_19.cpp "./ch11_Associative_Containers/exercise_11_19.cpp")|[练习11.20](exercise_11_20.cpp "./ch11_Associative_Containers/exercise_11_20.cpp")|[练习11.21](exercise_11_21.md "./ch11_Associative_Containers/exercise_11_21.md")|[练习11.22](exercise_11_22.md "./ch11_Associative_Containers/exercise_11_22.md")|[练习11.23](exercise_11_23.cpp "./ch11_Associative_Containers/exercise_11_23.cpp")|[练习11.24](exercise_11_24.md "./ch11_Associative_Containers/exercise_11_24.md")|
|[练习11.25](exercise_11_25.md "./ch11_Associative_Containers/exercise_11_25.md")|[练习11.26](exercise_11_26.md "./ch11_Associative_Containers/exercise_11_26.md")|[练习11.27](exercise_11_27.md "./ch11_Associative_Containers/exercise_11_27.md")|[练习11.28](exercise_11_28.cpp "./ch11_Associative_Containers/exercise_11_28.cpp")|[练习11.29](exercise_11_29.md "./ch11_Associative_Containers/exercise_11_29.md")|[练习11.30](exercise_11_30.md "./ch11_Associative_Containers/exercise_11_30.md")|
|[练习11.31](exercise_11_31.cpp "./ch11_Associative_Containers/exercise_11_31.cpp")|[练习11.32](exercise_11_32.cpp "./ch11_Associative_Containers/exercise_11_32.cpp")|[练习11.33](exercise_11_33.cpp "./ch11_Associative_Containers/exercise_11_33.cpp")|[练习11.34](exercise_11_34.md "./ch11_Associative_Containers/exercise_11_34.md")|[练习11.35](exercise_11_35.md "./ch11_Associative_Containers/exercise_11_35.md")|[练习11.36](exercise_11_36.md "./ch11_Associative_Containers/exercise_11_36.md")|
|[练习11.37](exercise_11_37.md "./ch11_Associative_Containers/exercise_11_37.md")|[练习11.38](exercise_11_38.cpp "./ch11_Associative_Containers/exercise_11_38.cpp")|

### 案例代码

- [初始化multiset(p377)](example_init_multiset.cpp)

- [创建pair对象的函数](example_make_pair.cpp)

- [使用map（p375）](example_map.cpp)

- [使用set（p375）](example_set.cpp)

- [无序容器对关键字的要求（p396）](example_unordered_set.cpp)

- [单词转换程序（p392）](example_word_transform.cpp)

## 第12章 动态内存

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习12.1](exercise_12_01.cpp "./ch12_Dynamic_Memory/exercise_12_01.cpp")|[练习12.2](exercise_12_02.md "./ch12_Dynamic_Memory/exercise_12_02.md")|[练习12.3](exercise_12_03.md "./ch12_Dynamic_Memory/exercise_12_03.md")|[练习12.4](exercise_12_04.md "./ch12_Dynamic_Memory/exercise_12_04.md")|[练习12.5](exercise_12_05.md "./ch12_Dynamic_Memory/exercise_12_05.md")|[练习12.6](exercise_12_06.cpp "./ch12_Dynamic_Memory/exercise_12_06.cpp")|
|[练习12.7](exercise_12_07.cpp "./ch12_Dynamic_Memory/exercise_12_07.cpp")|[练习12.8](exercise_12_08.md "./ch12_Dynamic_Memory/exercise_12_08.md")|[练习12.9](exercise_12_09.md "./ch12_Dynamic_Memory/exercise_12_09.md")|[练习12.10](exercise_12_10.md "./ch12_Dynamic_Memory/exercise_12_10.md")|[练习12.11](exercise_12_11.md "./ch12_Dynamic_Memory/exercise_12_11.md")|[练习12.12](exercise_12_12.md "./ch12_Dynamic_Memory/exercise_12_12.md")|
|[练习12.13](exercise_12_13.md "./ch12_Dynamic_Memory/exercise_12_13.md")|[练习12.14](exercise_12_14.md "./ch12_Dynamic_Memory/exercise_12_14.md")|[练习12.15](exercise_12_15.cpp "./ch12_Dynamic_Memory/exercise_12_15.cpp")|[练习12.16](exercise_12_16.cpp "./ch12_Dynamic_Memory/exercise_12_16.cpp")|[练习12.17](exercise_12_17.md "./ch12_Dynamic_Memory/exercise_12_17.md")|[练习12.18](exercise_12_18.md "./ch12_Dynamic_Memory/exercise_12_18.md")|
|[练习12.19](exercise_12_19.md "./ch12_Dynamic_Memory/exercise_12_19.md")|[练习12.20](exercise_12_20.cpp "./ch12_Dynamic_Memory/exercise_12_20.cpp")|[练习12.21](exercise_12_21.md "./ch12_Dynamic_Memory/exercise_12_21.md")|[练习12.22](exercise_12_22.md "./ch12_Dynamic_Memory/exercise_12_22.md")|[练习12.23](exercise_12_23.cpp "./ch12_Dynamic_Memory/exercise_12_23.cpp")|[练习12.24](exercise_12_24.cpp "./ch12_Dynamic_Memory/exercise_12_24.cpp")|
|[练习12.25](exercise_12_25.md "./ch12_Dynamic_Memory/exercise_12_25.md")|[练习12.26](exercise_12_26.cpp "./ch12_Dynamic_Memory/exercise_12_26.cpp")|[练习12.27](exercise_12_27.cpp "./ch12_Dynamic_Memory/exercise_12_27.cpp")|[练习12.28](exercise_12_28.cpp "./ch12_Dynamic_Memory/exercise_12_28.cpp")|[练习12.29](exercise_12_29.md "./ch12_Dynamic_Memory/exercise_12_29.md")|[练习12.30](exercise_12_30.md "./ch12_Dynamic_Memory/exercise_12_30.md")|
|[练习12.31](exercise_12_31.md "./ch12_Dynamic_Memory/exercise_12_31.md")|[练习12.32](exercise_12_32.cpp "./ch12_Dynamic_Memory/exercise_12_32.cpp")|[练习12.33](exercise_12_33.md "./ch12_Dynamic_Memory/exercise_12_33.md")|

### 案例代码

- [allocator类（427）](example_allocator.cpp)

- [使用智能指针共享底层数据(p405)](ch12_Dynamic_Memory/example_StrBlob)

- [weak_ptr的一个例子(p421)](ch12_Dynamic_Memory/example_StrBlobPtr)

- [文本查询程序(p430始)](ch12_Dynamic_Memory/example_TextQuery)

- [使用我们自己的释放操作（p416）](example_using_own_deletion.cpp)

## 第13章 拷贝控制

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习13.1](exercise_13_01.md "./ch13_Copy_Control/exercise_13_01.md")|[练习13.2](exercise_13_02.md "./ch13_Copy_Control/exercise_13_02.md")|[练习13.3](exercise_13_03.md "./ch13_Copy_Control/exercise_13_03.md")|[练习13.4](exercise_13_04.md "./ch13_Copy_Control/exercise_13_04.md")|[练习13.5](exercise_13_05.cpp "./ch13_Copy_Control/exercise_13_05.cpp")|[练习13.6](exercise_13_06.md "./ch13_Copy_Control/exercise_13_06.md")|
|[练习13.7](exercise_13_07.md "./ch13_Copy_Control/exercise_13_07.md")|[练习13.8](exercise_13_08.cpp "./ch13_Copy_Control/exercise_13_08.cpp")|[练习13.9](exercise_13_09.md "./ch13_Copy_Control/exercise_13_09.md")|[练习13.10](exercise_13_10.md "./ch13_Copy_Control/exercise_13_10.md")|[练习13.11](exercise_13_11.cpp "./ch13_Copy_Control/exercise_13_11.cpp")|[练习13.12](exercise_13_12.md "./ch13_Copy_Control/exercise_13_12.md")|
|[练习13.13](exercise_13_13.cpp "./ch13_Copy_Control/exercise_13_13.cpp")|[练习13.14](exercise_13_14.md "./ch13_Copy_Control/exercise_13_14.md")|[练习13.15](exercise_13_15.md "./ch13_Copy_Control/exercise_13_15.md")|[练习13.16](exercise_13_16.md "./ch13_Copy_Control/exercise_13_16.md")|[练习13.17](exercise_13_17.cpp "./ch13_Copy_Control/exercise_13_17.cpp")|[练习13.18](exercise_13_18.cpp "./ch13_Copy_Control/exercise_13_18.cpp")|
|[练习13.19](exercise_13_19.cpp "./ch13_Copy_Control/exercise_13_19.cpp")|[练习13.20](exercise_13_20.md "./ch13_Copy_Control/exercise_13_20.md")|[练习13.21](exercise_13_21.md "./ch13_Copy_Control/exercise_13_21.md")|[练习13.22](exercise_13_22.md "./ch13_Copy_Control/exercise_13_22.md")|[练习13.23](exercise_13_23.md "./ch13_Copy_Control/exercise_13_23.md")|[练习13.24](exercise_13_24.md "./ch13_Copy_Control/exercise_13_24.md")|
|[练习13.25](exercise_13_25.md "./ch13_Copy_Control/exercise_13_25.md")|[练习13.26](exercise_13_26.cpp "./ch13_Copy_Control/exercise_13_26.cpp")|[练习13.27](exercise_13_27.md "./ch13_Copy_Control/exercise_13_27.md")|[练习13.28](exercise_13_28.cpp "./ch13_Copy_Control/exercise_13_28.cpp")|[练习13.29](exercise_13_29.md "./ch13_Copy_Control/exercise_13_29.md")|[练习13.30](exercise_13_30.cpp "./ch13_Copy_Control/exercise_13_30.cpp")|
|[练习13.31](exercise_13_31.cpp "./ch13_Copy_Control/exercise_13_31.cpp")|[练习13.32](exercise_13_32.md "./ch13_Copy_Control/exercise_13_32.md")|[练习13.33](exercise_13_33.md "./ch13_Copy_Control/exercise_13_33.md")|[练习13.34](exercise_13_34.md "./ch13_Copy_Control/exercise_13_34.md")|[练习13.35](exercise_13_35.md "./ch13_Copy_Control/exercise_13_35.md")|[练习13.36](exercise_13_36.md "./ch13_Copy_Control/exercise_13_36.md")|
|[练习13.37](exercise_13_37.md "./ch13_Copy_Control/exercise_13_37.md")|[练习13.38](exercise_13_38.md "./ch13_Copy_Control/exercise_13_38.md")|[练习13.39](exercise_13_39.md "./ch13_Copy_Control/exercise_13_39.md")|[练习13.40](exercise_13_40.md "./ch13_Copy_Control/exercise_13_40.md")|[练习13.41](exercise_13_41.md "./ch13_Copy_Control/exercise_13_41.md")|[练习13.42](exercise_13_42.md "./ch13_Copy_Control/exercise_13_42.md")|
|[练习13.43](exercise_13_43.md "./ch13_Copy_Control/exercise_13_43.md")|[练习13.44](exercise_13_44.md "./ch13_Copy_Control/exercise_13_44.md")|[练习13.45](exercise_13_45.md "./ch13_Copy_Control/exercise_13_45.md")|[练习13.46](exercise_13_46.md "./ch13_Copy_Control/exercise_13_46.md")|[练习13.47](exercise_13_47.md "./ch13_Copy_Control/exercise_13_47.md")|[练习13.48](exercise_13_48.md "./ch13_Copy_Control/exercise_13_48.md")|
|[练习13.49](exercise_13_49.md "./ch13_Copy_Control/exercise_13_49.md")|[练习13.50](exercise_13_50.md "./ch13_Copy_Control/exercise_13_50.md")|[练习13.51](exercise_13_51.md "./ch13_Copy_Control/exercise_13_51.md")|[练习13.52](exercise_13_52.md "./ch13_Copy_Control/exercise_13_52.md")|[练习13.53](exercise_13_53.cpp "./ch13_Copy_Control/exercise_13_53.cpp")|[练习13.54](exercise_13_54.md "./ch13_Copy_Control/exercise_13_54.md")|
|[练习13.55](exercise_13_55.md "./ch13_Copy_Control/exercise_13_55.md")|[练习13.56](exercise_13_56.md "./ch13_Copy_Control/exercise_13_56.md")|[练习13.57](exercise_13_57.md "./ch13_Copy_Control/exercise_13_57.md")|[练习13.58](exercise_13_58.cpp "./ch13_Copy_Control/exercise_13_58.cpp")|

### 案例代码

- [拷贝控制示例（p460）](ch13_Copy_Control/example_Copy_Control)

- [使用引用计数的类（p456）](ch13_Copy_Control/example_Reference_Count)

- [引用限定符（p483）](ch13_Copy_Control/example_Reference_Qualifier)

- [自定义String简化版本（p470练习13.44）](ch13_Copy_Control/example_String)

- [动态内存管理类（p464）](ch13_Copy_Control/example_StrVec)

## 第14章 操作重载与类型转换

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习14.1](exercise_14_01.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_01.md")|[练习14.2](exercise_14_02.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_02.md")|[练习14.3](exercise_14_03.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_03.md")|[练习14.4](exercise_14_04.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_04.md")|[练习14.5](exercise_14_05.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_05.cpp")|[练习14.6](exercise_14_06.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_06.md")|
|[练习14.7](exercise_14_07.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_07.md")|[练习14.8](exercise_14_08.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_08.md")|[练习14.9](exercise_14_09.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_09.md")|[练习14.10](exercise_14_10.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_10.md")|[练习14.11](exercise_14_11.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_11.md")|[练习14.12](exercise_14_12.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_12.md")|
|[练习14.13](exercise_14_13.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_13.md")|[练习14.14](exercise_14_14.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_14.md")|[练习14.15](exercise_14_15.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_15.md")|[练习14.16](exercise_14_16.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_16.md")|[练习14.17](exercise_14_17.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_17.md")|[练习14.18](exercise_14_18.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_18.md")|
|[练习14.19](exercise_14_19.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_19.md")|[练习14.20](exercise_14_20.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_20.md")|[练习14.21](exercise_14_21.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_21.md")|[练习14.22](exercise_14_22.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_22.md")|[练习14.23](exercise_14_23.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_23.md")|[练习14.24](exercise_14_24.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_24.cpp")|
|[练习14.25](exercise_14_25.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_25.md")|[练习14.26](exercise_14_26.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_26.md")|[练习14.27](exercise_14_27.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_27.md")|[练习14.28](exercise_14_28.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_28.md")|[练习14.29](exercise_14_29.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_29.md")|[练习14.30](exercise_14_30.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_30.md")|
|[练习14.31](exercise_14_31.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_31.md")|[练习14.32](exercise_14_32.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_32.cpp")|[练习14.33](exercise_14_33.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_33.md")|[练习14.34](exercise_14_34.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_34.cpp")|[练习14.35](exercise_14_35.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_35.cpp")|[练习14.36](exercise_14_36.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_36.cpp")|
|[练习14.37](exercise_14_37.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_37.cpp")|[练习14.38](exercise_14_38.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_38.cpp")|[练习14.39](exercise_14_39.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_39.cpp")|[练习14.40](exercise_14_40.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_40.cpp")|[练习14.41](exercise_14_41.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_41.md")|[练习14.42](exercise_14_42.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_42.cpp")|
|[练习14.43](exercise_14_43.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_43.cpp")|[练习14.44](exercise_14_44.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_44.cpp")|[练习14.45](exercise_14_45.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_45.md")|[练习14.46](exercise_14_46.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_46.md")|[练习14.47](exercise_14_47.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_47.md")|[练习14.48](exercise_14_48.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_48.md")|
|[练习14.49](exercise_14_49.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_49.md")|[练习14.50](exercise_14_50.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_50.cpp")|[练习14.51](exercise_14_51.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_51.cpp")|[练习14.52](exercise_14_52.md "./ch14_Overloaded_Operations_and_Conversions/exercise_14_52.md")|[练习14.53](exercise_14_53.cpp "./ch14_Overloaded_Operations_and_Conversions/exercise_14_53.cpp")|

### 案例代码

- [定义含有类型转换运算符的类（p514）](example_conversion_operators.cpp)

- [函数调用运算符（p506）](example_function-call_operator.cpp)

- [可调用对象与function（p511）](example_function.cpp)

- [标准库定义的函数对象（p509）](example_std_funcobj.cpp)

## 第15章 面向对象程序设计

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习15.1](exercise_15_01.md "./ch15_Object-Oriented_Programming/exercise_15_01.md")|[练习15.2](exercise_15_02.md "./ch15_Object-Oriented_Programming/exercise_15_02.md")|[练习15.3](exercise_15_03.md "./ch15_Object-Oriented_Programming/exercise_15_03.md")|[练习15.4](exercise_15_04.md "./ch15_Object-Oriented_Programming/exercise_15_04.md")|[练习15.5](exercise_15_05.md "./ch15_Object-Oriented_Programming/exercise_15_05.md")|[练习15.6](exercise_15_06.cpp "./ch15_Object-Oriented_Programming/exercise_15_06.cpp")|
|[练习15.7](exercise_15_07.cpp "./ch15_Object-Oriented_Programming/exercise_15_07.cpp")|[练习15.8](exercise_15_08.md "./ch15_Object-Oriented_Programming/exercise_15_08.md")|[练习15.9](exercise_15_09.md "./ch15_Object-Oriented_Programming/exercise_15_09.md")|[练习15.10](exercise_15_10.md "./ch15_Object-Oriented_Programming/exercise_15_10.md")|[练习15.11](exercise_15_11.md "./ch15_Object-Oriented_Programming/exercise_15_11.md")|[练习15.12](exercise_15_12.md "./ch15_Object-Oriented_Programming/exercise_15_12.md")|
|[练习15.13](exercise_15_13.md "./ch15_Object-Oriented_Programming/exercise_15_13.md")|[练习15.14](exercise_15_14.md "./ch15_Object-Oriented_Programming/exercise_15_14.md")|[练习15.15](exercise_15_15.md "./ch15_Object-Oriented_Programming/exercise_15_15.md")|[练习15.16](exercise_15_16.cpp "./ch15_Object-Oriented_Programming/exercise_15_16.cpp")|[练习15.17](exercise_15_17.md "./ch15_Object-Oriented_Programming/exercise_15_17.md")|[练习15.18](exercise_15_18.cpp "./ch15_Object-Oriented_Programming/exercise_15_18.cpp")|
|[练习15.19](exercise_15_19.cpp "./ch15_Object-Oriented_Programming/exercise_15_19.cpp")|[练习15.20](exercise_15_20.md "./ch15_Object-Oriented_Programming/exercise_15_20.md")|[练习15.21](exercise_15_21.cpp "./ch15_Object-Oriented_Programming/exercise_15_21.cpp")|[练习15.22](exercise_15_22.md "./ch15_Object-Oriented_Programming/exercise_15_22.md")|[练习15.23](exercise_15_23.cpp "./ch15_Object-Oriented_Programming/exercise_15_23.cpp")|[练习15.24](exercise_15_24.md "./ch15_Object-Oriented_Programming/exercise_15_24.md")|
|[练习15.25](exercise_15_25.md "./ch15_Object-Oriented_Programming/exercise_15_25.md")|[练习15.26](exercise_15_26.md "./ch15_Object-Oriented_Programming/exercise_15_26.md")|[练习15.27](exercise_15_27.cpp "./ch15_Object-Oriented_Programming/exercise_15_27.cpp")|[练习15.28](exercise_15_28.md "./ch15_Object-Oriented_Programming/exercise_15_28.md")|[练习15.29](exercise_15_29.md "./ch15_Object-Oriented_Programming/exercise_15_29.md")|[练习15.30](exercise_15_30.md "./ch15_Object-Oriented_Programming/exercise_15_30.md")|
|[练习15.31](exercise_15_31.md "./ch15_Object-Oriented_Programming/exercise_15_31.md")|[练习15.32](exercise_15_32.md "./ch15_Object-Oriented_Programming/exercise_15_32.md")|[练习15.33](exercise_15_33.md "./ch15_Object-Oriented_Programming/exercise_15_33.md")|[练习15.34](exercise_15_34.md "./ch15_Object-Oriented_Programming/exercise_15_34.md")|[练习15.35](exercise_15_35.md "./ch15_Object-Oriented_Programming/exercise_15_35.md")|[练习15.36](exercise_15_36.md "./ch15_Object-Oriented_Programming/exercise_15_36.md")|
|[练习15.37](exercise_15_37.md "./ch15_Object-Oriented_Programming/exercise_15_37.md")|[练习15.38](exercise_15_38.md "./ch15_Object-Oriented_Programming/exercise_15_38.md")|[练习15.39](exercise_15_39.md "./ch15_Object-Oriented_Programming/exercise_15_39.md")|[练习15.40](exercise_15_40.md "./ch15_Object-Oriented_Programming/exercise_15_40.md")|[练习15.41](exercise_15_41.md "./ch15_Object-Oriented_Programming/exercise_15_41.md")|[练习15.42](exercise_15_42.md "./ch15_Object-Oriented_Programming/exercise_15_42.md")|

### 案例代码

- [抽象基类（p540）](example_abstract_base_class.cpp)

- [定义基类（p528）](example_base_class.cpp)

- [编写Basket类（容器与继承案例，p559）](example_basket.cpp)

- [派生类的拷贝控制成员（p554）](example_derived_class_copy_control_members.cpp)

- [定义派生类（p529）](example_derived_class.cpp)

- [文本查询程序再探(p562)](ch15_Object-Oriented_Programming/example_TextQuery)

## 第16章 模板与泛型编程

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习16.1](exercise_16_01.md "./ch16_Templates_and_GenericProgramming/exercise_16_01.md")|[练习16.2](exercise_16_02.md "./ch16_Templates_and_GenericProgramming/exercise_16_02.md")|[练习16.3](exercise_16_03.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_03.cpp")|[练习16.4](exercise_16_04.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_04.cpp")|[练习16.5](exercise_16_05.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_05.cpp")|[练习16.6](exercise_16_06.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_06.cpp")|
|[练习16.7](exercise_16_07.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_07.cpp")|[练习16.8](exercise_16_08.md "./ch16_Templates_and_GenericProgramming/exercise_16_08.md")|[练习16.9](exercise_16_09.md "./ch16_Templates_and_GenericProgramming/exercise_16_09.md")|[练习16.10](exercise_16_10.md "./ch16_Templates_and_GenericProgramming/exercise_16_10.md")|[练习16.11](exercise_16_11.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_11.cpp")|[练习16.12](exercise_16_12.md "./ch16_Templates_and_GenericProgramming/exercise_16_12.md")|
|[练习16.13](exercise_16_13.md "./ch16_Templates_and_GenericProgramming/exercise_16_13.md")|[练习16.14](exercise_16_14.md "./ch16_Templates_and_GenericProgramming/exercise_16_14.md")|[练习16.15](exercise_16_15.md "./ch16_Templates_and_GenericProgramming/exercise_16_15.md")|[练习16.16](exercise_16_16.md "./ch16_Templates_and_GenericProgramming/exercise_16_16.md")|[练习16.17](exercise_16_17.md "./ch16_Templates_and_GenericProgramming/exercise_16_17.md")|[练习16.18](exercise_16_18.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_18.cpp")|
|[练习16.19](exercise_16_19.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_19.cpp")|[练习16.20](exercise_16_20.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_20.cpp")|[练习16.21](exercise_16_21.md "./ch16_Templates_and_GenericProgramming/exercise_16_21.md")|[练习16.22](exercise_16_22.md "./ch16_Templates_and_GenericProgramming/exercise_16_22.md")|[练习16.23](exercise_16_23.md "./ch16_Templates_and_GenericProgramming/exercise_16_23.md")|[练习16.24](exercise_16_24.md "./ch16_Templates_and_GenericProgramming/exercise_16_24.md")|
|[练习16.25](exercise_16_25.md "./ch16_Templates_and_GenericProgramming/exercise_16_25.md")|[练习16.26](exercise_16_26.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_26.cpp")|[练习16.27](exercise_16_27.md "./ch16_Templates_and_GenericProgramming/exercise_16_27.md")|[练习16.28](exercise_16_28.md "./ch16_Templates_and_GenericProgramming/exercise_16_28.md")|[练习16.29](exercise_16_29.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_29.cpp")|[练习16.30](exercise_16_30.md "./ch16_Templates_and_GenericProgramming/exercise_16_30.md")|
|[练习16.31](exercise_16_31.md "./ch16_Templates_and_GenericProgramming/exercise_16_31.md")|[练习16.32](exercise_16_32.md "./ch16_Templates_and_GenericProgramming/exercise_16_32.md")|[练习16.33](exercise_16_33.md "./ch16_Templates_and_GenericProgramming/exercise_16_33.md")|[练习16.34](exercise_16_34.md "./ch16_Templates_and_GenericProgramming/exercise_16_34.md")|[练习16.35](exercise_16_35.md "./ch16_Templates_and_GenericProgramming/exercise_16_35.md")|[练习16.36](exercise_16_36.md "./ch16_Templates_and_GenericProgramming/exercise_16_36.md")|
|[练习16.37](exercise_16_37.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_37.cpp")|[练习16.38](exercise_16_38.md "./ch16_Templates_and_GenericProgramming/exercise_16_38.md")|[练习16.39](exercise_16_39.md "./ch16_Templates_and_GenericProgramming/exercise_16_39.md")|[练习16.40](exercise_16_40.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_40.cpp")|[练习16.41](exercise_16_41.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_41.cpp")|[练习16.42](exercise_16_42.md "./ch16_Templates_and_GenericProgramming/exercise_16_42.md")|
|[练习16.43](exercise_16_43.md "./ch16_Templates_and_GenericProgramming/exercise_16_43.md")|[练习16.44](exercise_16_44.md "./ch16_Templates_and_GenericProgramming/exercise_16_44.md")|[练习16.45](exercise_16_45.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_45.cpp")|[练习16.46](exercise_16_46.md "./ch16_Templates_and_GenericProgramming/exercise_16_46.md")|[练习16.47](exercise_16_47.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_47.cpp")|[练习16.48](exercise_16_48.md "./ch16_Templates_and_GenericProgramming/exercise_16_48.md")|
|[练习16.49](exercise_16_49.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_49.cpp")|[练习16.50](exercise_16_50.md "./ch16_Templates_and_GenericProgramming/exercise_16_50.md")|[练习16.51](exercise_16_51.md "./ch16_Templates_and_GenericProgramming/exercise_16_51.md")|[练习16.52](exercise_16_52.md "./ch16_Templates_and_GenericProgramming/exercise_16_52.md")|[练习16.53](exercise_16_53.md "./ch16_Templates_and_GenericProgramming/exercise_16_53.md")|[练习16.54](exercise_16_54.md "./ch16_Templates_and_GenericProgramming/exercise_16_54.md")|
|[练习16.55](exercise_16_55.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_55.cpp")|[练习16.56](exercise_16_56.md "./ch16_Templates_and_GenericProgramming/exercise_16_56.md")|[练习16.57](exercise_16_57.md "./ch16_Templates_and_GenericProgramming/exercise_16_57.md")|[练习16.58](exercise_16_58.md "./ch16_Templates_and_GenericProgramming/exercise_16_58.md")|[练习16.59](exercise_16_59.md "./ch16_Templates_and_GenericProgramming/exercise_16_59.md")|[练习16.60](exercise_16_60.md "./ch16_Templates_and_GenericProgramming/exercise_16_60.md")|
|[练习16.61](exercise_16_61.md "./ch16_Templates_and_GenericProgramming/exercise_16_61.md")|[练习16.62](exercise_16_62.md "./ch16_Templates_and_GenericProgramming/exercise_16_62.md")|[练习16.63](exercise_16_63.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_63.cpp")|[练习16.64](exercise_16_64.md "./ch16_Templates_and_GenericProgramming/exercise_16_64.md")|[练习16.65](exercise_16_65.cpp "./ch16_Templates_and_GenericProgramming/exercise_16_65.cpp")|[练习16.66](exercise_16_66.md "./ch16_Templates_and_GenericProgramming/exercise_16_66.md")|
|[练习16.67](exercise_16_67.md "./ch16_Templates_and_GenericProgramming/exercise_16_67.md")|

### 案例代码

- [自定义的make_shared，练习16.61](ch16_Templates_and_GenericProgramming/example_customer_MakeShared)

- [控制实例化（597）](ch16_Templates_and_GenericProgramming/example_explicit_instantiation)

- [函数模板（p578）](example_function_template.cpp)

- [成员模板（p595）](ch16_Templates_and_GenericProgramming/example_member_template)

- [重载与模板(p615)](example_overload_and_template.cpp)

- [窗口类的设计与实现(练习16.14, 16.15, p592)](ch16_Templates_and_GenericProgramming/example_Screen)

- [自己实现智能指针（p600, 练习16.28）](ch16_Templates_and_GenericProgramming/example_SmartPointer)

- [类模板（p583）](ch16_Templates_and_GenericProgramming/example_template_class)

- [编写可变参数函数模板（p620）](example_variadic_template2.cpp)

- [可变参数模板（p618）](example_variadic_template.cpp)

- [动态内存管理类（练习16.16，p592）](ch16_Templates_and_GenericProgramming/example_Vec)

## 第17章 标准库特殊设施

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习17.1](exercise_17_01.cpp "./ch17_Specialized_Library_Facilities/exercise_17_01.cpp")|[练习17.2](exercise_17_02.cpp "./ch17_Specialized_Library_Facilities/exercise_17_02.cpp")|[练习17.3](exercise_17_03.cpp "./ch17_Specialized_Library_Facilities/exercise_17_03.cpp")|[练习17.4](exercise_17_04.md "./ch17_Specialized_Library_Facilities/exercise_17_04.md")|[练习17.5](exercise_17_05.cpp "./ch17_Specialized_Library_Facilities/exercise_17_05.cpp")|[练习17.6](exercise_17_06.cpp "./ch17_Specialized_Library_Facilities/exercise_17_06.cpp")|
|[练习17.7](exercise_17_07.md "./ch17_Specialized_Library_Facilities/exercise_17_07.md")|[练习17.8](exercise_17_08.md "./ch17_Specialized_Library_Facilities/exercise_17_08.md")|[练习17.9](exercise_17_09.cpp "./ch17_Specialized_Library_Facilities/exercise_17_09.cpp")|[练习17.10](exercise_17_10.cpp "./ch17_Specialized_Library_Facilities/exercise_17_10.cpp")|[练习17.11](exercise_17_11.cpp "./ch17_Specialized_Library_Facilities/exercise_17_11.cpp")|[练习17.12](exercise_17_12.cpp "./ch17_Specialized_Library_Facilities/exercise_17_12.cpp")|
|[练习17.13](exercise_17_13.cpp "./ch17_Specialized_Library_Facilities/exercise_17_13.cpp")|[练习17.14](exercise_17_14.cpp "./ch17_Specialized_Library_Facilities/exercise_17_14.cpp")|[练习17.15](exercise_17_15.cpp "./ch17_Specialized_Library_Facilities/exercise_17_15.cpp")|[练习17.16](exercise_17_16.md "./ch17_Specialized_Library_Facilities/exercise_17_16.md")|[练习17.17](exercise_17_17.md "./ch17_Specialized_Library_Facilities/exercise_17_17.md")|[练习17.18](exercise_17_18.cpp "./ch17_Specialized_Library_Facilities/exercise_17_18.cpp")|
|[练习17.19](exercise_17_19.md "./ch17_Specialized_Library_Facilities/exercise_17_19.md")|[练习17.20](exercise_17_20.md "./ch17_Specialized_Library_Facilities/exercise_17_20.md")|[练习17.21](exercise_17_21.cpp "./ch17_Specialized_Library_Facilities/exercise_17_21.cpp")|[练习17.22](exercise_17_22.cpp "./ch17_Specialized_Library_Facilities/exercise_17_22.cpp")|[练习17.23](exercise_17_23.cpp "./ch17_Specialized_Library_Facilities/exercise_17_23.cpp")|[练习17.24](exercise_17_24.md "./ch17_Specialized_Library_Facilities/exercise_17_24.md")|
|[练习17.25](exercise_17_25.cpp "./ch17_Specialized_Library_Facilities/exercise_17_25.cpp")|[练习17.26](exercise_17_26.cpp "./ch17_Specialized_Library_Facilities/exercise_17_26.cpp")|[练习17.27](exercise_17_27.cpp "./ch17_Specialized_Library_Facilities/exercise_17_27.cpp")|[练习17.28](exercise_17_28.cpp "./ch17_Specialized_Library_Facilities/exercise_17_28.cpp")|[练习17.29](exercise_17_29.cpp "./ch17_Specialized_Library_Facilities/exercise_17_29.cpp")|[练习17.30](exercise_17_30.cpp "./ch17_Specialized_Library_Facilities/exercise_17_30.cpp")|
|[练习17.31](exercise_17_31.md "./ch17_Specialized_Library_Facilities/exercise_17_31.md")|[练习17.32](exercise_17_32.md "./ch17_Specialized_Library_Facilities/exercise_17_32.md")|[练习17.33](exercise_17_33.cpp "./ch17_Specialized_Library_Facilities/exercise_17_33.cpp")|[练习17.34](exercise_17_34.md "./ch17_Specialized_Library_Facilities/exercise_17_34.md")|[练习17.35](exercise_17_35.cpp "./ch17_Specialized_Library_Facilities/exercise_17_35.cpp")|[练习17.36](exercise_17_36.cpp "./ch17_Specialized_Library_Facilities/exercise_17_36.cpp")|
|[练习17.37](exercise_17_37.cpp "./ch17_Specialized_Library_Facilities/exercise_17_37.cpp")|[练习17.38](exercise_17_38.cpp "./ch17_Specialized_Library_Facilities/exercise_17_38.cpp")|[练习17.39](exercise_17_39.md "./ch17_Specialized_Library_Facilities/exercise_17_39.md")|

### 案例代码

- [格式化输入与输出（p666）](example_formatted_io.cpp)

- [使用子表达式验证电话号码（p654）](example_phone_number.cpp)

- [随机数（p659）](example_random.cpp)

- [指定或使用正则表达式时的错误（p648）](example_regex_error.cpp)

- [使用regex_replace（p657）](example_regex_replace.cpp)

- [seek和tell函数（p676）](example_seek_tell.cpp)

- [匹配与Regex迭代器类型（p650）](example_sregex_iterator.cpp)

- [使用tuple返回多个值（p638）](example_tuple.cpp)

- [未格式化IO（p673）](example_unformatted_io.cpp)

- [使用正则表达式库（p646）](example_using_regex.cpp)

- [使用子表达式（p653）](example_using_subexpression.cpp)

## 第18章 用于大型程序的工具

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习18.1](exercise_18_01.cpp "./ch18_Tools_for_Large_Programs/exercise_18_01.cpp")|[练习18.2](exercise_18_02.md "./ch18_Tools_for_Large_Programs/exercise_18_02.md")|[练习18.3](exercise_18_03.cpp "./ch18_Tools_for_Large_Programs/exercise_18_03.cpp")|[练习18.4](exercise_18_04.md "./ch18_Tools_for_Large_Programs/exercise_18_04.md")|[练习18.5](exercise_18_05.cpp "./ch18_Tools_for_Large_Programs/exercise_18_05.cpp")|[练习18.6](exercise_18_06.cpp "./ch18_Tools_for_Large_Programs/exercise_18_06.cpp")|
|[练习18.7](exercise_18_07.md "./ch18_Tools_for_Large_Programs/exercise_18_07.md")|[练习18.8](exercise_18_08.md "./ch18_Tools_for_Large_Programs/exercise_18_08.md")|[练习18.9](exercise_18_09.md "./ch18_Tools_for_Large_Programs/exercise_18_09.md")|[练习18.10](exercise_18_10.md "./ch18_Tools_for_Large_Programs/exercise_18_10.md")|[练习18.11](exercise_18_11.md "./ch18_Tools_for_Large_Programs/exercise_18_11.md")|[练习18.12](exercise_18_12.md "./ch18_Tools_for_Large_Programs/exercise_18_12.md")|
|[练习18.13](exercise_18_13.md "./ch18_Tools_for_Large_Programs/exercise_18_13.md")|[练习18.14](exercise_18_14.cpp "./ch18_Tools_for_Large_Programs/exercise_18_14.cpp")|[练习18.15](exercise_18_15.md "./ch18_Tools_for_Large_Programs/exercise_18_15.md")|[练习18.16](exercise_18_16.cpp "./ch18_Tools_for_Large_Programs/exercise_18_16.cpp")|[练习18.17](exercise_18_17.md "./ch18_Tools_for_Large_Programs/exercise_18_17.md")|[练习18.18](exercise_18_18.cpp "./ch18_Tools_for_Large_Programs/exercise_18_18.cpp")|
|[练习18.19](exercise_18_19.md "./ch18_Tools_for_Large_Programs/exercise_18_19.md")|[练习18.20](exercise_18_20.cpp "./ch18_Tools_for_Large_Programs/exercise_18_20.cpp")|[练习18.21](exercise_18_21.cpp "./ch18_Tools_for_Large_Programs/exercise_18_21.cpp")|[练习18.22](exercise_18_22.cpp "./ch18_Tools_for_Large_Programs/exercise_18_22.cpp")|[练习18.23](exercise_18_23.cpp "./ch18_Tools_for_Large_Programs/exercise_18_23.cpp")|[练习18.24](exercise_18_24.md "./ch18_Tools_for_Large_Programs/exercise_18_24.md")|
|[练习18.25](exercise_18_25.cpp "./ch18_Tools_for_Large_Programs/exercise_18_25.cpp")|[练习18.26](exercise_18_26.cpp "./ch18_Tools_for_Large_Programs/exercise_18_26.cpp")|[练习18.27](exercise_18_27.cpp "./ch18_Tools_for_Large_Programs/exercise_18_27.cpp")|[练习18.28](exercise_18_28.cpp "./ch18_Tools_for_Large_Programs/exercise_18_28.cpp")|[练习18.29](exercise_18_29.cpp "./ch18_Tools_for_Large_Programs/exercise_18_29.cpp")|[练习18.30](exercise_18_30.cpp "./ch18_Tools_for_Large_Programs/exercise_18_30.cpp")|

## 第19章 特殊工具与技术

|A|L|L|E|X|E|
| :-: | :-: | :-: | :-: | :-: | :-: |
|[练习19.1](exercise_19_01.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_01.cpp")|[练习19.2](exercise_19_02.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_02.md")|[练习19.3](exercise_19_03.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_03.cpp")|[练习19.4](exercise_19_04.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_04.cpp")|[练习19.5](exercise_19_05.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_05.md")|[练习19.6](exercise_19_06.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_06.cpp")|
|[练习19.7](exercise_19_07.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_07.cpp")|[练习19.8](exercise_19_08.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_08.cpp")|[练习19.9](exercise_19_09.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_09.md")|[练习19.10](exercise_19_10.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_10.cpp")|[练习19.11](exercise_19_11.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_11.md")|[练习19.12](exercise_19_12.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_12.cpp")|
|[练习19.13](exercise_19_13.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_13.cpp")|[练习19.14](exercise_19_14.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_14.cpp")|[练习19.15](exercise_19_15.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_15.md")|[练习19.16](exercise_19_16.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_16.md")|[练习19.17](exercise_19_17.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_17.md")|[练习19.18](exercise_19_18.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_18.cpp")|
|[练习19.19](exercise_19_19.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_19.cpp")|[练习19.20](exercise_19_20.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_20.md")|[练习19.21](exercise_19_21.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_21.md")|[练习19.22](exercise_19_22.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_22.md")|[练习19.23](exercise_19_23.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_23.md")|[练习19.24](exercise_19_24.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_24.md")|
|[练习19.25](exercise_19_25.md "./ch19_Specialized_Tools_and_Techniques/exercise_19_25.md")|[练习19.26](exercise_19_26.cpp "./ch19_Specialized_Tools_and_Techniques/exercise_19_26.cpp")|

### 案例代码

- [链接指示，extern "C"（758）](ch19_Specialized_Tools_and_Techniques/example_extern_C)

- [成员指针函数表（p743）](example_function_table.cpp)

- [将成员函数用作可调用对象（p744）](example_memfunc_as_callable_obj.cpp)

- [定位new表达式（p729）](example_placement_new.cpp)

- [类成员指针（p739）](example_pointer_to_member.cpp)

- [type_info类（p735）](example_type_info.cpp)

- [含有类类型成员的union（p750）](example_union_with_class.cpp)

- [使用RTTI（p733）](example_use_RTTI.cpp)

