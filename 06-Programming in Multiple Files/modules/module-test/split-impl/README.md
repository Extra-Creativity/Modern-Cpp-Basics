我们把module interface中的实现分离到module implementation中，module interface只有一个（用export module的开头表示），而module implementation可以有任意多个（用module开头表示，无export）。

具体地，即`Person.mpp`是module interface，其他两个是module implementation。

我们这里只是一个例子，这种文件长度用一个文件作为module implementation就够了。