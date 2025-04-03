# Hệ thống theo dõi sức khỏe khi vận động

## Mô tả

......

## Sử dụng (cho server flask)

### Tạo venv (optional)

```bash
python -m venv myenv
source myenv/bin/activate  # Linux/Mac
myenv\Scripts\activate     # Windows
```

### Cài các thư viện cần thiết

```bash
pip install -r .\pyserver\requirements.txt
pip install flask-socketio
# Cho server thu thập dữ liệu
pip install -r .\datac_requirements.txt
```

### Khởi chạy server

```bash
python pyserver/app.py
```

### Hiển thị dữ liệu thu được

```bash
py .\read_and_plot_sensor_data.py
```

## Server thu thập dữ liệu (dataC)

### Khởi chạy server (Development)

Để khởi động server trong môi trường phát triển:

```bash
# Di chuyển vào thư mục dataC
cd dataC
# Khởi động server
python server.py
```

### Khởi chạy server (Production)

```bash
# Sử dụng Gunicorn
cd dataC
gunicorn -w 4 -b 0.0.0.0:8888 wsgi:application
```

### Triển khai trên Render.com

1. Tạo tài khoản trên Render.com
2. Tạo một Web Service mới, liên kết với repository của bạn
3. Cấu hình như sau:
    - Root Directory: `dataC`
    - Build Command: `pip install -r ../datac_requirements.txt`
    - Start Command: `gunicorn -w 4 wsgi:application`
    - Environment Variables:
        - `PORT=10000` (hoặc port được Render cung cấp)
        - `TCP_PORT=10001` (cổng khác cho TCP server)
4. Bật chức năng "Private Service" nếu bạn muốn dịch vụ riêng tư

#### Lưu ý quan trọng về cổng khi triển khai

-   Render.com thường chỉ định cổng 10000 cho ứng dụng web, không phải cổng 8888
-   TCP_PORT (8889) cần được đặt thành cổng khác được Render cho phép (10001)
-   ESP8266 cần được cập nhật để kết nối với domain của Render và cổng TCP mới (10001)

```cpp
// Cập nhật trong file ESP8266
const char* serverIP = "your-app.onrender.com"; // Tên miền Render
const int serverPort = 10001; // Cổng TCP mới
```

### Triển khai với Systemd (Linux)

1. Tạo file service:

```bash
sudo nano /etc/systemd/system/oxisensor.service
```

2. Thêm nội dung sau:

```ini
[Unit]
Description=OxiSensor Data Collection Service
After=network.target

[Service]
User=your_username
WorkingDirectory=/path/to/OxiSensor/dataC
ExecStart=/path/to/venv/bin/gunicorn -w 4 -b 0.0.0.0:8888 wsgi:application
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

3. Kích hoạt và khởi động service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable oxisensor
sudo systemctl start oxisensor
```

### Bắt đầu đo

```bash
curl -X POST http://localhost:8888/start
```

### Dừng đo

```bash
curl -X POST http://localhost:8888/stop
```

### Xem và tải các file đã ghi

Truy cập `http://localhost:8888/files` để xem và tải các file đã ghi.

### Xem dashboard

Truy cập `http://localhost:8888/` để xem bảng điều khiển trực quan.

### Xem phân tích dữ liệu thu thập mới nhất

Truy cập `http://localhost:8888/latest-analysis` để xem phân tích của bộ dữ liệu mới nhất.

### Thiết lập IP server cho ESP8266

Trong file `config.h`, thay thế `your_domain_name.com` bằng địa chỉ IP hoặc tên miền của server.

```cpp
const char* serverIP = "your_domain_name.com"; // Replace with your domain name
```

### Kiểm tra và mở cổng 8888 và 8889

Đảm bảo rằng cổng 8888 (HTTP) và 8889 (TCP) được mở và có thể truy cập được. Sử dụng lệnh sau để kiểm tra:

```bash
nc -zv oxinheartbeat.phuongy.works 8888
nc -zv oxinheartbeat.phuongy.works 8889
```

Nếu cổng không mở, hãy cập nhật quy tắc tường lửa hoặc cấu hình máy chủ để cho phép kết nối đến các cổng này:

```bash
# Mở cổng với ufw (Ubuntu)
sudo ufw allow 8888/tcp
sudo ufw allow 8889/tcp
```

## Cấu trúc mã nguồn (dataC)

```
dataC/
├── __init__.py                # Định nghĩa package
├── server.py                  # Entry point cho development & ứng dụng Flask
├── wsgi.py                    # Entry point WSGI cho production
├── data_decoder.py            # Xử lý giải mã dữ liệu cảm biến
├── sampling_analyzer.py       # Phân tích tốc độ lấy mẫu
├── templates/                 # Templates HTML
│   ├── analysis.html          # Trang phân tích dữ liệu
│   ├── files.html             # Trang danh sách các file
│   └── index.html             # Trang chính/dashboard
└── data/                      # Thư mục chứa dữ liệu
    ├── raw/                   # Dữ liệu thô
    ├── csv/                   # Dữ liệu đã xử lý
    └── analysis/              # Kết quả phân tích
```
