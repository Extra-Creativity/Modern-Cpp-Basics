# Answer

# Filesystem

1. ```c++
   template<typename... Args>
   stdfs::path Join(Args&&... args)
   {
       return (stdfs::path{} / ... / args);
   }
   ```

2. 