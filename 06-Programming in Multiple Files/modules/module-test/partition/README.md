这一部分是partition的例子，partition可以是interface的，也可以是implementation的。不过注意，interface的partition和primary interface有明显不同的地方：
+ 其他模块可以直接import primary interface，但不能import该interface的特定partition。只有当primary interface对interface partition进行export import时，它所export的东西才实际对外可见。
+ interface partition和implementation partition不应同名（即`export module A:B`不能与`module A:B`同时出现，后者也不是前者的实现文件；如果想给partition进行实现的分离，应该使用新的名字，例如`module A:B.impl`，随后在`A:B`中`import :B.impl`）。而primary interface可以有很多implementation，它们以相同的module为名字。

在Visual Studio编译时，你可能需要把“扫描源以查找模块依赖关系”设置为“是”来成功编译。不过目前VS的Intellisense还有bug，所以会有一些“错误的报错”，实际上是可以编译的。不是特别确定是否符合标准。