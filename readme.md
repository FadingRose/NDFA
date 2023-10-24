# 形式语言与自动机 大作业

## A 读取上下文无关文法 (File1.txt)

- 使用文件读取操作来获取File1.txt中定义的上下文无关文法。


### A.1 如何确定一个文法 "是上下文无关文法？"

上下文无关文法（Context-Free Grammar, CFG）的产生式（规则）都有以下形式：

$$
A \rightarrow \alpha
$$

其中，$A$是一个非终结符（Non-terminal），$\alpha$是由非终结符和终结符（Terminal）组成的字符串。如果一个文法的所有产生式都遵循这种形式，那么它就是一个上下文无关文法。

具体地，上下文无关文法的特点有：
- 左侧只有一个非终结符
- 右侧是由非终结符和终结符组成的任意字符串，也可以是空（$\epsilon$）

### A.2 我们应当用怎样的数据结构来描述一个文法$G$

描述上下文无关文法$G$通常需要以下四个组成部分：
- 一组非终结符$N$
- 一组终结符$T$
- 一组产生式$P$
- 一个开始符号$S$，它是$N$中的一个元素


在 C++ 中：

```cpp
struct CFG {
    std::set<char> non_terminals;
    std::set<char> terminals;
    std::map<char, std::vector<std::string>> productions;
    char start_symbol;
    char epsilon_symbol = '@';
};
```

## B 将其转化为 Greibach 标准形式 (File2.txt)

### B.1 消除无用符号

- 遍历产生式来确定哪些符号是无法到达或者不能导出任何终结符的，然后从文法中删除这些符号。
>**有用符号**
>一个非终结符$X \in V$被称为有用符号，如果存在某个字符串$w$属于该文法$G$生成的语言$L(G)$，并且在从起始符号$S$推导到$w$的过程中，这个非终结符$X$至少出现了一次。
>
>数学上，这可以表示为：存在$\alpha, \beta \in (V \cup T)^*$，使得$S$可以通过一系列推导步骤变为$\alpha X \beta$，并且$\alpha X \beta$可以进一步推导出$w$。
>
>**无用符号**
>
>相反地，如果一个非终结符$X$在任何$w \in L(G)$的推导过程中都不出现，则$X$被称为无用符号。

#### B.1.a 消除不可达符号


#### B.1.b 消除不产生终结符的符号

```c++
    // Step 2 Eliminate Non-Terminating Symbols
    std::set<char> terminating;
    changed = true;
    while (changed) {
        changed = false;
        for (const auto& production : cfg.productions) {
            for (const auto& rhs : production.second) {
                // For a specific production, we check the right-hand first,
                // True :: c is a Terminal || c is not a Terminal and has been marked in the terminating
                if (std::all_of(rhs.begin(), rhs.end(), [&](char c) {
                    return !isNonterminal(c) || terminating.find(c) != terminating.end();
                    })) {
                    if (terminating.find(production.first) == terminating.end()) {
                        terminating.insert(production.first);
                        changed = true;
                    }
                }
            }
        }
    }
```

考虑文法：

```
B -> bA
A -> a
```
> `a` 被放入 `terminating` :: `!isNonterminal('a') is True`
> `A` 被放入 `terminating` :: `A` 可以推导出右侧仅为 `terminal`，因此 `A` 被放入 `terminating` 

### B.2 消除单一产生式

- 查找所有的单一产生式（如 `A -> B`），然后用`B`能推导出的所有字符串来替换`A`。

### B.3 消除$\epsilon$规则

- 找出所有可能生成$\epsilon$的非终结符，并更新所有包含这些非终结符的规则。

### B.4 消除左递归

- 对于形如 `A -> Aa | b` 的规则，将其改写为避免左递归。

### B.5 转化为 Greibach 标准形式

- 最后，确保每个产生式都是以一个终结符开始。

## C 构建 NFDA（非确定性有限自动机）

- 根据 Greibach 标准形式的产生式构建 NFDA。通常，每个产生式都会转换为自动机中的一个状态转移。

### C.1 输入任意字符串（File3.txt），基于该 NFDA 判断是否接受该字符串

- 读取File3.txt中的输入字符串。
- 使用NFDA进行模拟，以确定是否可以接受该字符串。


## D 测试样例

### D.1 消除无用符号

#### D.1.a 全部符号都是有用的

```plain text
S -> A
A -> a
```

#### D.1.b 有无用符号

```plain text
S -> A | B | D
A -> a
B -> b
C -> c
D -> E
```

### D.2 消除单一表达式

#### D.2.a 没有单一表达式

```plain text
S -> Aa | Bb
A -> a
B -> b
```

#### D.2.b 含有单一表达式

```plain text
S -> A
A -> B
B -> a
```

### D.3 消除空表达式

#### D.3.a 没有空表达式

```plain text
S -> a | b
```

#### D.3.a 含有空表达式

```plain text
S -> A | @
A -> a | @
```

### D.4 消除直接左递归

#### D.4.a 没有直接左递归

```plain text
S -> Aa | Bb
A -> a
B -> b
```

#### D.4.b 含有直接左递归

```plain text
S -> Sa | Sb | ε
```

### D.5 消除间接左递归

#### D.5.a 没有间接左递归

```plain text
S -> Aa | Bb
A -> a
B -> b
```

#### D.5.b 含有间接左递归

```plain text
S -> Aa
A -> Sb | a
```


S -> aAbBC | abBC
A -> bcB | Cca | a | aA
B -> bcB | Cca
C -> cC | c
