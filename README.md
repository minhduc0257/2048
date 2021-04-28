# 2048

## Tính năng
- Logic cơ bản của trò chơi 2048
- Sinh thêm số nếu trong 5s không có lượt di chuyển nào

## Yêu cầu hệ thống
- Một trình biên dịch được CMake hỗ trợ. Bản thân trình biên dịch này phải hỗ trợ C++20.
  - Chương trình đã được thử nghiệm với GCC 10.2.0 và Clang++ 11.1.0 trên Linux.

## Hướng dẫn biên dịch
Chạy các lệnh sau lần lượt từ gốc của thư mục chứa mã nguồn :
- `mkdir -p build`
- `cd build`
- `cmake ..`
- `cmake --build .`

Folder `build` sẽ chứa tập tin thực thi của trò chơi.