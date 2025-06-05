# Tài liệu Luồng Mã

Tài liệu này cung cấp giải thích chi tiết về luồng mã cho module `realtime-tracking`, bao gồm cả mã nhúng ESP8266 và máy chủ.

## Mã Nhúng ESP8266 (ehtracking)

### Luồng Khởi Tạo

1. **Khởi Động Hệ Thống**:

    - Khởi tạo giao tiếp nối tiếp để gỡ lỗi.
    - Thiết lập đèn LED RGB để chỉ báo trạng thái.
    - Cấu hình cảm biến MAX30102 để thu thập dữ liệu.
    - Khởi tạo mô hình TensorFlow Lite để phân loại hoạt động.
    - Thiết lập kết nối WiFi sử dụng thông tin trong `config.h`.
    - Kết nối với máy chủ TCP để truyền dữ liệu.

2. **Tạo Tác Vụ**:

    - Tạo Tác Vụ Cảm Biến để thu thập dữ liệu với đệm kép.
    - Tạo Tác Vụ Xử Lý để phân tích và truyền dữ liệu.
    - Tạo Tác Vụ Giám Sát Trạng Thái để theo dõi sức khỏe hệ thống.

3. **Quản Lý Trạng Thái**:
    - Hệ thống chuyển từ STATE_INIT sang STATE_IDLE nếu khởi tạo thành công.
    - Cập nhật màu đèn LED để chỉ báo trạng thái hệ thống hiện tại.

### Luồng Vòng Lặp Chính

1. **Quản Lý WiFi**:

    - Duy trì kết nối WiFi.
    - Tự động kết nối lại nếu mất kết nối.

2. **Xử Lý Lệnh**:

    - Kiểm tra lệnh TCP đến từ máy chủ.
    - Xử lý lệnh để bắt đầu/dừng giám sát, thay đổi cài đặt, v.v.

3. **Chỉ Báo Trạng Thái**:
    - Cập nhật màu đèn LED dựa trên trạng thái hệ thống hiện tại.
    - Cung cấp phản hồi trực quan về hoạt động của hệ thống.

### Luồng Tác Vụ Cảm Biến

1. **Thu Thập Dữ Liệu**:
    - Lấy mẫu dữ liệu từ cảm biến MAX30102 với tần số cấu hình.
    - Thực hiện đệm kép để ngăn mất dữ liệu trong quá trình xử lý.
    - Báo hiệu Tác Vụ Xử Lý khi đệm sẵn sàng.

### Luồng Tác Vụ Xử Lý

1. **Phân Tích Dữ Liệu**:

    - Xử lý dữ liệu cảm biến thô để trích xuất các chỉ số sức khỏe.
    - Chạy phân loại hoạt động bằng mô hình TensorFlow Lite.
    - Tính toán các chỉ số sức khỏe và chỉ báo trạng thái.

2. **Truyền Dữ Liệu**:
    - Đóng gói dữ liệu đã xử lý thành định dạng JSON.
    - Gửi dữ liệu đến máy chủ qua kết nối TCP.
    - Xử lý lỗi truyền và logic thử lại.

## Máy Chủ (ehtrackingserver)

### Luồng Tiếp Nhận Dữ Liệu

1. **Lắng Nghe Kết Nối**:

    - Lắng nghe kết nối đến trên cổng TCP được chỉ định.
    - Nhận các gói dữ liệu JSON từ thiết bị ESP8266.
    - Phân tích và xác thực dữ liệu đến.

2. **Xử Lý Dữ Liệu**:

    - Xử lý dữ liệu chỉ số sức khỏe đến.
    - Theo dõi phân loại hoạt động.
    - Duy trì dữ liệu lịch sử để trực quan hóa.

3. **Giao Diện Web**:

    - Cung cấp bảng điều khiển trực quan hóa thời gian thực.
    - Cập nhật biểu đồ và chỉ báo khi có dữ liệu mới.
    - Hiển thị kết quả phân loại sức khỏe và hoạt động.

4. **Giao Diện Lệnh**:
    - Cho phép gửi lệnh đến thiết bị ESP8266 kết nối.
    - Cung cấp tùy chọn cấu hình cho các thông số giám sát.
    - Cho phép điều khiển từ xa hệ thống theo dõi.

### Sơ Đồ Luồng Dữ Liệu

```
+-------------+     +---------------+     +----------------+
| MAX30102    |---->| Double Buffer |---->| Data Analysis  |
| Sensor      |     | System        |     | (TensorFlow)   |
+-------------+     +---------------+     +----------------+
                                                 |
                                                 v
+-----------------+     +---------------+     +----------------+
| Web Dashboard   |<----| Server        |<----| TCP/IP         |
| Visualization   |     | Processing    |     | Transmission   |
+-----------------+     +---------------+     +----------------+
```

## Tham Khảo

-   [README Chính](../README.md)
-   [Mã Nhúng ESP8266](../ehtracking/README.md)
-   [Tài Liệu Máy Chủ](../ehtrackingserver/README.md)
